import sys,time,random
import json

from PyQt5.QtCore import *
from PyQt5.QtNetwork import *

import qtutil
from qtoxkit import *
from toxtunutils import *
# from srudp import *


class ToxTunCli(QObject):
    def __init__(self, parent = None):
        super(ToxTunCli, self).__init__(parent)       
        return

    
class ToxTunFileCli(ToxTunCli):
    def __init__(self, config_file = './toxtun.ini', parent = None):
        super(ToxTunFileCli, self).__init__(parent)
        # self.cfg = ToxTunConfig('./toxtun.ini')
        self.cfg = ToxTunConfig(config_file)
        self.toxkit = None # QToxKit
        self.tcpsrvs = {}  # id => srv
        self.cons = {}     # peer => con
        self.chans = {}   # sock => chan, chanocli_%d => chan
        self.chano = 8
        self.cmdno = 7
        
        return

    def start(self):
        self._startToxNet()
        self._startTcpServer()
        qDebug('started')
        return

    def _startToxNet(self):
        tkname = self.cfg.srvname
        toxkit = QToxKit('toxcli', True)

        toxkit.connected.connect(self._toxnetConnected, Qt.QueuedConnection)
        toxkit.disconnected.connect(self._toxnetDisconnected, Qt.QueuedConnection)
        toxkit.friendAdded.connect(self._toxnetFriendAdded, Qt.QueuedConnection)
        toxkit.friendConnectionStatus.connect(self._toxnetFriendConnectionStatus, Qt.QueuedConnection)
        toxkit.friendConnected.connect(self._toxnetFriendConnected, Qt.QueuedConnection)
        toxkit.newMessage.connect(self._toxnetFriendMessage, Qt.QueuedConnection)
        toxkit.fileRecv.connect(self._toxnetFileRecv, Qt.QueuedConnection)
        toxkit.fileRecvControl.connect(self._toxnetFileRecvControl, Qt.QueuedConnection)
        toxkit.fileRecvChunk.connect(self._toxnetFileRecvChunk, Qt.QueuedConnection)
        toxkit.fileChunkRequest.connect(self._toxnetFileChunkRequest, Qt.QueuedConnection)
        
        self.toxkit = toxkit

        return

    def _startTcpServer(self):
        i = 0
        for rec in self.cfg.recs:
            srv = QTcpServer()
            srv.newConnection.connect(self._onNewTcpConnection, Qt.QueuedConnection)
            ok = srv.listen(QHostAddress.Any, rec.local_port)
            qDebug(str(ok))
            self.tcpsrvs[i] = srv
            self.tcpsrvs[srv] = rec
            i = i + 1
        return


    def _toxnetConnected(self):
        qDebug('here')
        toxsock = self.sender()

        i = 0
        for rec in self.cfg.recs:
            con = ToxConnection()
            con.srv = self.tcpsrvs[i]
            con.peer = rec.remote_pubkey
            self.cons[con.peer] = con
            self.cons[con.srv] = con

            self._toxnetTryFriendAdd(con, i)
            # self.toxkit.friendAdd(con.peer, 'from toxtun cli %d' % i)
            i = i + 1
            
        return
    
    def _toxnetDisconnected(self):
        qDebug('here')
        return

    def _toxnetTryFriendAdd(self, con, idx):
        qDebug(str(con.peer))
        
        friend_number = 0
        add_norequest = False
        friend_request_msg = 'from toxtun cli %d@%s' % (idx, self.cfg.srvname)

        try:
            # friend_number = self.toxkit.friendAddNorequest(con.peer)
            pass
        except Exception as e:
            qDebug(str(e.args))

       
        try:
            friend_number = self.toxkit.friendAdd(con.peer, friend_request_msg)
        except Exception as e:
            # print(e)
            qDebug(str(e.args))
            add_norequest = True

        if True or add_norequest is True:
            try:
                friend_number = self.toxkit.friendAddNorequest(con.peer)
            except Exception as e:
                qDebug(str(e.args))
        

        return

    def _toxnetFriendAdded(self, fid):
        qDebug('hehe:fid=' + fid)
        con = self.cons[fid]

        return

    def _toxnetFriendConnectionStatus(self, fid, status):

        return
    def _toxnetFriendConnected(self, friend_id):
        qDebug('herhe:fid=' + friend_id)

        # con = self.cons[fid]
        con = None
        for id in self.cons:
            if type(id) == str and id[0:64] == friend_id:
                con = self.cons[id]
                break

        if con is None:
            qDebug('warning con not found')
            return

        if con.self_file_number == -1:
            idprefix = friend_id[0:6]
            file_name = 'toxtunfilecli_%s_toxfilesock' % idprefix
            file_size = pow(2, 56)
            file_number = self.toxkit.fileSend(con.peer, file_size, file_name)
            con.self_file_number = file_number
            con.reqchunks = []   # reset，在给self_file_number重置时，也要重置reqchunks
            qDebug('new cli file %d' % file_number)
            self_file_to_con_key = 'file_to_con_%d_%s' % (file_number, friend_id)
            self.cons[self_file_to_con_key] = con
        else:
            ### tests send ping
            qDebug('maybe resue offlined file channel')
            chan = None
            for k in self.chans:
                if type(k) == QTcpSocket and self.chans[k].con == con:
                    chan = self.chans[k]
                    break
            if chan is None:
                qDebug('chan about this con not found')
            else:
                pkt = {'chcli': chan.chanocli , 'chsrv': chan.chanosrv, 'type': 'PING', 'ttl':1}
                jspkt = json.JSONEncoder().encode(pkt)
                fjspkt = '%1371s' % jspkt
                bret = self.toxkit.fileControl(con.peer, con.self_file_number, 0)
                qDebug(str(bret))
                if len(con.reqchunks) > 0:
                    reqinfo = con.reqchunks.pop(0)
                    bret = self.toxkit.fileSendChunk(con.peer, con.self_file_number, reqinfo[2], fjspkt)
                else: qDebug('no free chunks found')

        return

    def _toxnetFileRecv(self, friend_id, file_number, file_size, file_name):
        qDebug(file_name)

        # con = self.cons[friend_id]
        con = None
        for id in self.cons:
            if type(id) == str and id[0:64] == friend_id:
                con = self.cons[id]
                break

        if con is None:
            qDebug('warning con not found')
            return

        curr_self_file_number = con.self_file_number
        if con.peer_file_number != -1:
            qDebug('warning, maybe reuse file channel')


        con.peer_file_number = file_number
        peer_file_to_con_key = 'peer_file_to_con_%d_%s' % (file_number, friend_id)
        self.cons[peer_file_to_con_key] = peer_file_to_con_key

        self.toxkit.fileControl(friend_id, file_number, 0)
        
        # segs = file_name.split('_')
        # qDebug(str(segs))

        # cmdno = int(segs[1])
        # host = segs[2]
        # port = int(segs[3])

        # chan = self.chans['cmdno_%d'%cmdno]
        # self.chans[file_name] = chan
        # chan.peer_file_number = file_number

        # peer_file_to_chan_key = 'peer_file_to_chan_%d_%s' % (file_number, friend_id[0:64])
        # self.chans[peer_file_to_chan_key] = chan
        
        # self.toxkit.fileControl(friend_id, file_number, 0)
        return


    def _toxnetFileRecvControl(self, friend_id, file_number, control):
        qDebug('here')
        print((friend_id, file_number, control))

        if control == 0:
            qDebug('resumed')
            # self.toxkit.fileControl(friend_id, file_number, 1)
        elif control == 1:
            qDebug('paused')
        elif control == 2:
            qDebug('canceled')

        return
    
    def _toxnetFileRecvChunk(self, friend_id, file_number, position, data):
        import random
        if random.randrange(0,9) == 1: qDebug('here')

        jspkt = data.lstrip()
        pkt = json.JSONDecoder().decode(jspkt)
        ptype = pkt['type']
        # qDebug(str(jspkt))

        peer_file_to_con_key = 'peer_file_to_con_%d_%s' % (file_number, friend_id[0:64])
        con = self.cons[peer_file_to_con_key]
        # print(friend_id, file_number, position, data)

        if ptype == 'CONNECT':
            qDebug(str(jspkt))
            chanocli = pkt['chcli']
            chan = self.chans['chanocli_%d' % chanocli]
            chan.chanosrv = pkt['chsrv']
            qDebug('real connected, can read write now')
            chan.sock.readyRead.emit()
            pass
        elif ptype == 'CLOSE':
            qDebug(str(jspkt))
            chanocli = pkt['chcli']
            chanokey = 'chanocli_%d' % chanocli
            #chan = self.chans['chanocli_%d' % chanocli]

            if chanokey in self.chans:
                chan = self.chans[chanokey]
                chan.peer_close = True
                con = chan.con
            
                fjspkt = '%1371s' % jspkt
                if len(con.reqchunks) > 0:
                    reqinfo = con.reqchunks.pop(0)
                    bret = self.toxkit.fileSendChunk(con.peer, con.self_file_number, reqinfo[2], fjspkt)

                self._toxchanPromiseCleanup(chan)
            else:
                chanosrv = pkt['chsrv']
                qDebug('no assoc chan found: %d/%d' % (chanocli, chanosrv))
                
            pass
        elif ptype == 'WRITE':
            chanocli = pkt['chcli']
            chanokey = 'chanocli_%d' % chanocli
            # chan = self.chans['chanocli_%d' % chanocli]
            if chanokey in self.chans:
                chan = self.chans[chanokey]  
                self._tcpWrite(chan, pkt['data'])
            else:
                chanosrv = pkt['chsrv']
                qDebug('chan not exist: %d/%d' % (chanocli, chanosrv))
            pass
        else: qDebug('ptype error: %s' % ptype)
        
        # # qDebug(str(data))
        # plen = data[0:4].lstrip()
        # plen = int(plen)
        # pkt = data[5:].lstrip()
        # qDebug('%d, %s' % (plen, pkt))

        # self._tcpWrite(chan, pkt)
        
        return

    def _toxnetFileChunkRequest(self, friend_id, file_number, position, length):
        import random
        if random.randrange(0,9) == 1:
            qDebug('here')
            qDebug('%s, %d, %d, %d' % (friend_id, file_number, position, length))

        self_file_to_con_key = 'file_to_con_%d_%s' % (file_number, friend_id)
        con = self.cons[self_file_to_con_key]

        reqinfo = (friend_id, file_number, position, length)
        # qDebug(str(reqinfo))
        con.reqchunks.append(reqinfo)
        
        # self_file_to_chan_key = 'self_file_to_chan_%d_%s' % (file_number, friend_id[0:64])
        # chan = self.chans[self_file_to_chan_key]

        # reqinfo = (friend_id, file_number, position, length)
        # qDebug(str(reqinfo))
        
        # chan.reqchunks.append(reqinfo)
        # qDebug('reqchunks len: %d' % len(chan.reqchunks))

        # if chan.sock.bytesAvailable() > 0: chan.sock.readyRead.emit()
        self._toxnetReadyWrite()
        
        return

    def _toxnetFriendMessage(self, friendId, msg):
        qDebug(friendId)
        return

    def _toxnetReadyWrite(self):
        candi_chans = []
        for ck in self.chans:
            chan = self.chans[ck]
            if type(ck) == QTcpSocket:  # 排除重复chan功能。
                if chan.sock.bytesAvailable() > 0: candi_chans.append(chan)

        if len(candi_chans) > 0:
            n = random.randrange(0, len(candi_chans))
            chan = candi_chans[n]
            chan.sock.readyRead.emit()

        return
    
    # @param data bytes | QByteArray
    def _toxnetWrite(self, chan, data):
        cmdno = self._nextCmdno()
        chan.cmdno = cmdno

        qDebug('here')
        extra = {'cmdno': cmdno, 'chano': chan.chano}
        res = chan.rudp.buf_send_pkt(data, extra)
        
        # if type(data) == bytes: data = QByteArray(data)
        # msg = {'cmd': 'write', 'cmdno': cmdno, 'chano': chan.chano, 'data': data.toHex().data().decode('utf8'),}

        # msg = json.JSONEncoder().encode(msg)
        # self.toxkit.sendMessage(chan.con.peer, msg)
        return

    def _toxchanConnected(self):
        qDebug('here')
        udp = self.sender()
        chan = self.chans[udp]
        while chan.sock.bytesAvailable() > 0:
            bcc = chan.sock.read(128)
            print('first cli data chrunk: ', bcc)
            self._toxnetWrite(chan, bcc)
            # data = QByteArray(bcc).toHex().data().decode('utf8')
            # extra = {'chano': chan.chano}

            # jspkt = udp.buf_send_pkt(data, extra)
            # self.toxkit.sendMessage(chan.con.peer, jspkt)
        
        return

    def _toxchanDisconnected(self):
        qDebug('here')
        udp = self.sender()
        chan = self.chans[udp]
        sock = chan.sock
        
        # 清理资源
        if sock not in self.chans: qDebug('sock maybe already closed')
        else: self.chans.pop(sock)

        if udp not in self.chans: qDebug('udp maybe already closed')
        else: self.chans.pop(udp)

        chano = chan.chano
        if chano not in self.chans: qDebug('maybe already closed222')
        else: self.chans.pop(chano)

        qDebug('chans size: %d' % len(self.chans))
        
        return

    def _toxchanReadyRead(self):
        qDebug('here')
        udp = self.sender()
        chan = self.chans[udp]
        
        while True:
            opkt = udp.readPacket()
            if opkt is None: break
            self._tcpWrite(chan, opkt.data)
            
        return

    def _toxchanCanClose(self):
        qDebug('here')
        udp = self.sender()
        chan = self.chans[udp]

        cmdno = self._nextCmdno()
        extra = {'cmd': 'close', 'chano': chan.chano, 'cmdno': cmdno,}
        res = chan.rudp.mkdiscon(extra)
        return

    def _toxchanPromiseCleanup(self, chan):
        sock = chan.sock
        chanocli_key = 'chanocli_%d' % chan.chanocli

        promise_results = {
            'peer_close': chan.peer_close,
            # 'ctrl_close': chan.ctrl_close,
            'sock_close': chan.sock_close,
        }

        promise_result = True
        for pk in promise_results: promise_result = promise_result and promise_results[pk]

        if promise_result is True:
            qDebug('promise satisfied: %d<=>%d.' % (chan.chanocli, chan.chanosrv))
        else:
            qDebug('promise noooooot satisfied: %d<=>%d.' % (chan.chanocli, chan.chanosrv))
            qDebug(str(promise_results))
            return

        # if chan.peer_close is True and chan.ctrl_close is True: pass
        # else: return

        # 清理资源
        if sock not in self.chans: qDebug('sock maybe already closed')
        else: self.chans.pop(sock)

        sock.disconnected.disconnect()
        sock.close()

        if chanocli_key not in self.chans: qDebug('chcli maybe already closed')
        else: self.chans.pop(chanocli_key)

        qDebug('chans size: %d' % len(self.chans))
        
        return

    def _nextChano(self):
        self.chano = self.chano +2
        return self.chano

    def _nextCmdno(self):
        self.cmdno = self.cmdno +1
        return self.cmdno
    
    def _onNewTcpConnection(self):
        qDebug('here')
        srv = self.sender()
        rec = self.tcpsrvs[srv]

        sock = srv.nextPendingConnection()
        sock.readyRead.connect(self._onTcpReadyRead, Qt.QueuedConnection)
        sock.disconnected.connect(self._onTcpDisconnected, Qt.QueuedConnection)

        con = self.cons[srv]
        chan = ToxTunFileChannel(con, sock)
        chan.host = rec.remote_host
        chan.port = rec.remote_port
        chan.chanocli = self._nextChano()
        self.chans['chanocli_%d' % chan.chanocli] = chan
        self.chans[sock] = chan
        qDebug('sock enter chans')
        

        # pkt = {'chcli': , 'chsrv':, 'type':, 'data':, 'host':, 'port':,}
        # type = CONNECT|WRITE|CLOSE
        pkt = {'chcli': chan.chanocli , 'chsrv': 0, 'type': 'CONNECT', 'data':'',
               'host': chan.host, 'port': chan.port, }
        jspkt = json.JSONEncoder().encode(pkt)
        fjspkt = '%1371s' % jspkt

        if len(con.reqchunks) > 0:
            reqinfo = con.reqchunks.pop(0)
            bret = self.toxkit.fileSendChunk(con.peer, con.self_file_number, reqinfo[2], fjspkt)
            qDebug(str(bret))
            if bret is False:
                # what error? how deal this case?
                # pytox.OperationFailedError: tox_file_send_chunk() failed:5
                pass
        else:
            qDebug('no free chunk found')

        # file_name = 'toxtunfilecli_%d_%s_%d_toxfilesock' % (chan.cmdno, chan.host, chan.port)
        # file_size = pow(2, 56)
        # file_number = self.toxkit.fileSend(con.peer, file_size, file_name)
        # chan.self_file_number = file_number
        # self.chans[file_name] = chan
        # qDebug('new cli file %d' % file_number)

        # self_file_to_chan_key = 'self_file_to_chan_%d_%s' % (file_number, con.peer[0:64])
        # self.chans[self_file_to_chan_key] = chan


        
        # transport = ToxTunTransport(self.toxkit, con.peer)
        # chan.transport = transport
        
        # udp = Srudp()
        # udp.setTransport(transport)
        # chan.rudp = udp
        # self.chans[udp] = chan
        # udp.connected.connect(self._toxchanConnected, Qt.QueuedConnection)
        # udp.disconnected.connect(self._toxchanDisconnected, Qt.QueuedConnection)
        # udp.readyRead.connect(self._toxchanReadyRead, Qt.QueuedConnection)
        # udp.canClose.connect(self._toxchanCanClose, Qt.QueuedConnection)

        # extra = {'cmdno': chan.cmdno, 'host': chan.host, 'port': chan.port}

        # res = udp.mkcon(extra)
                
        return

    def _onTcpDisconnected(self):
        qDebug('here')
        sock = self.sender()
        if sock not in self.chans:
            qDebug('maybe already closed')
            return

        chan = self.chans[sock]
        con = chan.con

        pkt = {'chcli': chan.chanocli , 'chsrv': chan.chanosrv, 'type': 'CLOSE', 'data':''}
        jspkt = json.JSONEncoder().encode(pkt)
        fjspkt = '%1371s' % jspkt

        # if len(con.reqchunks) > 0:
        #    reqinfo = con.reqchunks.pop(0)
        #    bret = self.toxkit.fileSendChunk(con.peer, con.self_file_number, reqinfo[2], fjspkt)
        #    qDebug(str(bret))

        #
        chan.sock_close = True
        self._toxchanPromiseCleanup(chan)
            
        # chan = self.chans[sock]
        # chano = chan.chano
        # qDebug(chan.debugInfo())
        
        # if chano not in self.chans:
        #     qDebug('maybe already closed222')
        #     self.chans.pop(sock)
        #     return
        
        # cmdno = self._nextCmdno()
        # extra = {'cmd': 'close', 'chano': chan.chano, 'cmdno': cmdno,}
        # res = chan.rudp.mkdiscon(extra)
        # chan.transport.closed = True
        
        # jspkt = chan.rudp.mkdiscon(extra)
        # self.toxkit.sendMessage(chan.con.peer, jspkt)

        return
    
    def _onTcpReadyRead(self):
        qDebug('here')
        sock = self.sender()
        chan = self.chans[sock]
        con = chan.con

        if chan.chanosrv == 0:
            qDebug('maybe not connected or already closed.')
            return

        peekSize = 128  # 897  # 987时就有可能导致超长拆包发送
        cnter = 0
        tlen = 0
        
        while sock.bytesAvailable() > 0 and len(con.reqchunks) > 0:
            bcc = chan.sock.peek(peekSize)
            print(bcc)
            reqinfo = con.reqchunks[0]
            qDebug(str(reqinfo))
            pkt = {'chcli': chan.chanocli, 'chsrv': chan.chanosrv, 'type': 'WRITE', 'data':''}
            rawpkt = QByteArray(bcc).toBase64().data().decode('utf8')
            pkt['data'] = rawpkt
            jspkt = json.JSONEncoder().encode(pkt)
            fullpkt = '%1371s' % (jspkt)
            bret = self.toxkit.fileSendChunk(con.peer, con.self_file_number, reqinfo[2], fullpkt)
            # self._toxnetWrite(chan, bcc)
            if bret is True:
                reqinfo = con.reqchunks.pop(0)
                bcc1 = chan.sock.read(peekSize)
                chan.rdlen += len(bcc)
                cnter += 1
                tlen += len(bcc)
            qDebug('%s, bret=%d' % (str(len(fullpkt)), bret))

            
        # while sock.bytesAvailable() > 0 and len(chan.reqchunks) > 0:
        #     bcc = chan.sock.read(128)
        #     print(bcc)
        #     reqinfo = chan.reqchunks.pop(0)
        #     qDebug(str(reqinfo))
        #     rawpkt = QByteArray(bcc).toBase64().data().decode('utf8')
        #     fullpkt = '%4d$%1366s' % (len(rawpkt), rawpkt)
        #     bret = self.toxkit.fileSendChunk(con.peer, chan.self_file_number, reqinfo[2], fullpkt)
        #     # self._toxnetWrite(chan, bcc)
        #     chan.rdlen += len(bcc)
        #     cnter += 1
        #     tlen += len(bcc)
        #     qDebug('%s, bret=%d' % (str(len(fullpkt)), bret))
        

        # qDebug(str(chan.chano))
        # if chan.chano == 0: return

        # cnter = 0
        # tlen = 0
        # while sock.bytesAvailable() > 0:
        #     bcc = chan.sock.read(128)
        #     print(bcc)
        #     self._toxnetWrite(chan, bcc)
        #     chan.rdlen += len(bcc)
        #     cnter += 1
        #     tlen += len(bcc)

        qDebug('XDR: sock->toxnet: %d/%d, %d' % (tlen, chan.rdlen, cnter))
        return

    
    
    def _tcpWrite(self, chan, data):
        sock = chan.sock
        chan = self.chans[sock]
        
        # qDebug('netsize: %d, %s' % (len(data), str(data)))
        if type(data) == str: data = data.encode('utf8')
        rawdata = QByteArray.fromBase64(data)
        # rawdata = chan.transport.decodeData(data)
        # qDebug('rawsize: %d, %s' % (len(rawdata), str(rawdata)))
        print(rawdata[len(rawdata)-50:])
        
        n = sock.write(rawdata)
        chan.wrlen += n
        qDebug('XDR: toxnet->sock: %d/%d' % (n, chan.wrlen))
        
        return


def main():
    app = QCoreApplication(sys.argv)
    qtutil.pyctrl()

    config_file = './toxtun.ini'
    if len(sys.argv) == 2:
        config_file = sys.argv[1]
        if not os.path.exists(config_file):
            print('config file is not exists: %s' % config_file)
            help()
            sys.exit(1)
    elif len(sys.argv) == 1:
        pass
    else:
        print('provide a valid config file please')
        help()
        sys.exit(1)

    
    tuncli = ToxTunFileCli(config_file)
    tuncli.start()
    
    app.exec_()
    return

if __name__ == '__main__': main()


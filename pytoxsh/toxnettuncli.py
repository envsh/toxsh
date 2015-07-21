import sys, os, time
import json

from PyQt5.QtCore import *
from PyQt5.QtNetwork import *

import qtutil
from qtoxkit import *
from toxtunutils import *
from srudp import *
from httpserver import *


class ToxNetTunCli(QObject):
    def __init__(self, config_file = './toxtun.ini', parent = None):
        super(ToxNetTunCli, self).__init__(parent)
        # self.cfg = ToxTunConfig('./toxtun_whttp.ini')
        self.cfg = ToxTunConfig(config_file)
        self.toxkit = None # QToxKit
        self.tcpsrvs = {}  # id => srv
        self.cons = {}     # peer => con
        self.chans = {}   # sock => chan, rudp => chan
        # self.cmdno = 0
        self.chano = 7  # step=2, 奇数
        
        # debug/manager console server
        self.httpd = QHttpServer()
        self.httpd.newRequest.connect(self._mcsrv_newRequest, Qt.QueuedConnection)

        return

    def start(self):
        self._startToxNet()
        self._startTcpServer()
        qDebug('started')

        ###
        self.httpd.listen(8114, QHostAddress.LocalHost)
        return

    def _startToxNet(self):
        # toxkit = QToxKit('toxcli', False)
        toxkit = QToxKit('toxcli', True)
        
        toxkit.connected.connect(self._toxnetConnected, Qt.QueuedConnection)
        toxkit.disconnected.connect(self._toxnetDisconnected, Qt.QueuedConnection)
        toxkit.friendAdded.connect(self._toxnetFriendAdded, Qt.QueuedConnection)
        toxkit.friendConnectionStatus.connect(self._toxnetFriendConnectionStatus, Qt.QueuedConnection)
        toxkit.friendConnected.connect(self._toxnetFriendConnected, Qt.QueuedConnection)
        toxkit.newMessage.connect(self._toxnetFriendMessage, Qt.QueuedConnection)
        
        self.toxkit = toxkit

        return

    def _startTcpServer(self):
        i = 0
        for rec in self.cfg.recs:
            srv = QTcpServer()
            srv.newConnection.connect(self._onNewTcpConnection, Qt.QueuedConnection)
            ok = srv.listen(QHostAddress.LocalHost, rec.local_port)
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
            i += 1
            pass

        return
    
    def _toxnetDisconnected(self):
        qDebug('here')
        return

    def _toxnetTryFriendAdd(self, con, idx):
       friend_number = 0
       add_norequest = False
       friend_request_msg = 'from toxtun cli %d@%s' % (idx, self.cfg.srvname)

       try:
           friend_number = self.toxkit.friendAddNorequest(con.peer)
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
        qDebug('hehe:' + fid)
        con = self.cons[fid]

        
        return

    def _toxnetFriendConnectionStatus(self, fid, status):

        return
    def _toxnetFriendConnected(self, fid):
        qDebug('herhe:' + fid)
        
        return

    def _hotfix_resp_srvfin1(self, peer, opkt):
        qDebug('here')
        
        ropkt = SruPacket2()
        ropkt.msg_type = 'SRVFIN2'
        ropkt.extra = opkt.extra

        # 不知道peer是谁啊，怎么回包呢？
        self.toxkit.sendMessage(peer, ropkt.encode())
        # res = self.transport.send(ropkt.encode())
        time.sleep(0.0001)
        self.toxkit.sendMessage(peer, ropkt.encode())
        # res = self.transport.send(ropkt.encode())
        #self.peer_closed = True
        # self.peerClosed.emit()
        qDebug('emit disconnect event.')
        # self.disconnected.emit()
        return

    def _toxnetFriendMessage(self, friendId, msg):
        qDebug(friendId)

        tmsg = json.JSONDecoder().decode(msg)
        # qDebug(str(tmsg))
        
        # dispatch的过程
        opkt = SruPacket2.decode(msg)
        if opkt.msg_type == 'SYN2':
            jmsg = opkt.extra
            # cmdnokey = 'cmdno_%d' % jmsg['cmdno']
            # chan = self.chans[cmdnokey]
            chan = self.chans[jmsg['chcli']]
            chan.chanosrv = jmsg['chsrv']
            # self.chans[chan.chano] = chan
            # self.chans.pop(cmdnokey)
            # chan.rudp.chano = chan.chano
            if chan.chanocli == 0 or chan.chanosrv == 0:
                qDebug('not possible')
                sys.exit(-1)
            
            res = chan.rudp.buf_recv_pkt(msg)
            # ropkt = chan.rudp.buf_recv_pkt(msg)
            # self.toxkit.sendMessage(chan.con.peer, ropkt)

            pass
        elif opkt.msg_type == 'DATA':
            jmsg = opkt.extra
            chan = self.chans[jmsg['chcli']]
            res = chan.rudp.buf_recv_pkt(msg)
            # jspkt = chan.rudp.buf_recv_pkt(msg)
            # self.toxkit.sendMessage(chan.con.peer, jspkt)
            qDebug('here')
            pass
        elif opkt.msg_type == 'SRVFIN':
            jmsg = opkt.extra
            chano = jmsg['chcli']
            if chano not in self.chans:
                qDebug('warning chano not exists: %d, hotfix it' % (chano))
                ### fix chan not exists
                # chan.rudp.transport.send
                # sys.exit(1)
                # hotfix this problem
                self._hotfix_resp_srvfin1(friendId, opkt)
            else:
                chan = self.chans[chano]
                res = chan.rudp.buf_recv_pkt(msg)
                chan.rudp.startCheckClose()
            # ropkt = chan.rudp.buf_recv_pkt(msg)
            # if ropkt is not None: self.toxkit.sendMessage(chan.con.peer, ropkt)
        else:
            jmsg = opkt.extra
            chano = jmsg['chcli']
            if chano not in self.chans:
                qDebug('warning chano not exists: %d, drop it' % (chano))
            else:
                chan = self.chans[jmsg['chcli']]
                chan.rudp.buf_recv_pkt(msg)
            qDebug('here')
        
        # dispatch的过程        
        #if jmsg['cmd'] == 'write':
        #    chan = self.chans[jmsg['chano']]
        #    self._tcpWrite(chan, jmsg['data'])
        #    pass

        #if jmsg['cmd'] == 'close':
        #    chano = jmsg['chano']
        #    if chano in self.chans:
        #        chan = self.chans[chano]
        #        self.chans.pop(chano)
        #        chan.sock.close()
        #    pass

        #有可能产生消息积压，如果调用端不读取的话
        return

    # @param data bytes | QByteArray
    def _toxnetWrite(self, chan, data):
        # cmdno = self._nextCmdno()
        # chan.cmdno = cmdno

        qDebug('here')
        extra = {'cmdno': 0, 'chsrv': chan.chanosrv, 'chcli': chan.chanocli}
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

        if chan.sock.bytesAvailable() > 0:
            chan.sock.readyRead.emit()
            
        # while chan.sock.bytesAvailable() > 0:
        #     bcc = chan.sock.read(128)
        #     print('first cli data chrunk: ', bcc)
        #     self._toxnetWrite(chan, bcc)

            
        #     # data = QByteArray(bcc).toHex().data().decode('utf8')
        #     # extra = {'chano': chan.chano}
            
        #     # jspkt = udp.buf_send_pkt(data, extra)
        #     # self.toxkit.sendMessage(chan.con.peer, jspkt)
        
        return

    def _toxchanDisconnected(self):
        qDebug('here')
        udp = self.sender()

        if udp not in self.chans:
            qDebug('maybe already cleanuped: %d' % udp.chano)
            return
        
        chan = self.chans[udp]
        sock = chan.sock

        chan.rudp_close = True
        self._toxchanPromiseCleanup(chan)
        return
        
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

        # cmdno = self._nextCmdno()
        extra = {'cmd': 'close', 'chsrv': chan.chanosrv, 'chcli': chan.chanocli, 'cmdno': 0,}
        res = chan.rudp.mkdiscon(extra)  # 永不主动发起关闭
        return

    def _toxchanPeerClosed(self):
        qDebug('here')
        udp = self.sender()
        chan = self.chans[udp]
        chan.peer_close = True

        self._toxchanPromiseCleanup(chan)
        
        return

    def _toxchanTimeWaitTimeout(self):
        qDebug('here')
        udp = self.sender()
        chan = self.chans[udp]

        self._toxchanPromiseCleanup(chan)
        return
    
    # promise原理的优雅关闭与清理
    def _toxchanPromiseCleanup(self, chan):
        qDebug('here')
        
        # sock 是否关闭
        # peer 是否关闭
        # srudp的状态是否是CLOSED
        # srudp的TIME_WAIT状态是否超时了

        chan.peer_close = chan.rudp.peer_closed
        promise_results = {
            'peer_close': chan.peer_close,
            'sock_close': chan.sock_close,
            'rudp_close': chan.rudp_close,
        }
        
        nowtime = QDateTime.currentDateTime()
        if chan.rudp.begin_close_time is not None:
            qDebug(str(chan.rudp.begin_close_time.msecsTo(nowtime)))
        if chan.rudp.self_passive_close is False:
            promise_results['active_state'] = (chan.rudp.state == 'TIME_WAIT')
            if chan.rudp.begin_close_time is None:
                promise_results['time_wait_timeout'] = False                
            else:
                duration = chan.rudp.begin_close_time.msecsTo(nowtime)
                promise_results['time_wait_timeout'] = (duration > 15000)
        else:
            promise_results['pasv_state'] = (chan.rudp.state == 'CLOSED')

        promise_result = True
        for pk in promise_results: promise_result = promise_result and promise_results[pk]
        
        if promise_result is True:
            qDebug('promise satisfied: %d<=>%d.' % (chan.chanocli, chan.chanosrv))
        else:
            qDebug('promise noooooot satisfied: %d<=>%d.' % (chan.chanocli, chan.chanosrv))
            qDebug(str(promise_results))
            return

        sock = chan.sock
        udp = chan.rudp

        udp.step_send_timer.stop()
        
        # 清理资源
        if sock not in self.chans: qDebug('sock maybe already closed')
        else: self.chans.pop(sock)

        if udp not in self.chans: qDebug('udp maybe already closed')
        else: self.chans.pop(udp)

        chano = chan.chanocli
        if chano not in self.chans: qDebug('maybe already closed222')
        else: self.chans.pop(chano)

        qDebug('chans size: %d' % len(self.chans))

        return


    def _nextChano(self):
        self.chano = self.chano +2
        return self.chano

    
    def _nextCmdno(self):
        # self.cmdno = self.cmdno +1
        # return self.cmdno
        return
    
    def _onNewTcpConnection(self):
        srv = self.sender()
        rec = self.tcpsrvs[srv]

        ###TODO if friend is not connected: reset this request
        if srv not in self.cons:
            qDebug('maybe toxnet not ready.')

        sock = srv.nextPendingConnection()
        sock.readyRead.connect(self._onTcpReadyRead)
        sock.disconnected.connect(self._onTcpDisconnected)

        con = self.cons[srv]
        chan = ToxChannel(con, sock)
        chan.host = rec.remote_host
        chan.port = rec.remote_port
        chan.chanocli = self._nextChano()
        # chan.cmdno = self._nextCmdno()
        self.chans[chan.chanocli] = chan
        # self.chans['cmdno_%d' % chan.cmdno] = chan
        self.chans[sock] = chan

        transport = ToxTunTransport(self.toxkit, con.peer)
        chan.transport = transport
        
        udp = Srudp()
        udp.setTransport(transport)
        udp.chano = chan.chanocli
        chan.rudp = udp
        self.chans[udp] = chan
        udp.connected.connect(self._toxchanConnected, Qt.QueuedConnection)
        udp.disconnected.connect(self._toxchanDisconnected, Qt.QueuedConnection)
        udp.readyRead.connect(self._toxchanReadyRead, Qt.QueuedConnection)
        udp.canClose.connect(self._toxchanCanClose, Qt.QueuedConnection)

        extra = {'chcli': chan.chanocli, 'cmdno': 0, 'host': chan.host, 'port': chan.port}

        res = udp.mkcon(extra)
        # pkt = udp.mkcon(extra)        
        # self.toxkit.sendMessage(con.peer, pkt)
                
        return

    def _onTcpDisconnected(self):
        qDebug('here')
        sock = self.sender()
        if sock not in self.chans:
            qDebug('maybe already closed')
            return

        chan = self.chans[sock]
        chan.sock_close = True
        chan.transport.closed = True
        # chan.rudp.startCheckClose()
        self._toxchanPromiseCleanup(chan)

        return
        
        chan = self.chans[sock]
        chano = chan.chano
        qDebug(chan.debugInfo())
        
        if chano not in self.chans:
            qDebug('maybe already closed222')
            self.chans.pop(sock)
            return
        
        # cmdno = self._nextCmdno()
        extra = {'cmd': 'close', 'chano': chan.chano, 'cmdno': 0,}
        # res = chan.rudp.mkdiscon(extra)
        chan.transport.closed = True
        
        # jspkt = chan.rudp.mkdiscon(extra)
        # self.toxkit.sendMessage(chan.con.peer, jspkt)

        return
    
    def _onTcpReadyRead(self):
        qDebug('here')
        sock = self.sender()
        chan = self.chans[sock]

        qDebug(str(chan.chanocli))
        qDebug(str(chan.chanosrv))
        if chan.chanocli <= 0:
            qDebug('not possible')
            sys.exit(-1)
            return

        if chan.chanosrv <= 0:
            qDebug('not connected channel: %d/%d' % (chan.chanocli, chan.chanosrv))
            return

        cnter = 0
        tlen = 0
        while sock.bytesAvailable() > 0:
            bcc = chan.sock.read(128)
            print(bcc)
            self._toxnetWrite(chan, bcc)
            chan.rdlen += len(bcc)
            cnter += 1
            tlen += len(bcc)

        qDebug('XDR: sock->toxnet: %d/%d, %d' % (tlen, chan.rdlen, cnter))
        # bcc = sock.readAll()
        # toxsock.write(bcc)
        return

    
    def _tcpWrite(self, chan, data):
        sock = chan.sock
        chan = self.chans[sock]
        # qDebug('netsize: %d, %s' % (len(data), str(data)))
        # rawdata = QByteArray.fromHex(data)
        rawdata = chan.transport.decodeData(data)
        # qDebug('rawsize: %d, %s' % (len(rawdata), str(rawdata)))
        
        n = sock.write(rawdata)
        chan.wrlen += n
        qDebug('XDR: toxnet->sock: %d/%d' % (n, chan.wrlen))
        
        return

    #####
    def _mcsrv_newRequest(self, req, resp):
        qDebug('here')
        # resp.setHeader('Content-Length', '12')
        # resp.writeHead(200)
        # resp.write('12345\n12345\n')

        cclen = 12
        ccs = []
        ccs.append('12345\n12345\n')


        cc0 = 'chan num: %d/%.2f' % (len(self.chans), len(self.chans)/3.0) + "\n"
        cclen += len(cc0)
        ccs.append(cc0)

        cnter = 0
        for ck in self.chans:
            if type(ck) != QTcpSocket: continue
            chan = self.chans[ck]
            promise_results = self._toxchanPromiseResults(chan)
            # 这儿发现cli端有chan-0出现，有可能是根本没有收到服务器端的SYN2响应包，也就是丢失包了。
            # 也就是建立连接失败的请求，才会出现这种问题。需要连接超时判断。
            # 而服务器端则没有出现chan-0的现象。
            cc0 = '%d chcli-%d/%d: ' % (cnter, chan.chanocli, chan.chanosrv) + str(promise_results) + "\n"
            cclen += len(cc0)
            # resp.write(cc0)
            ccs.append(cc0)
            cnter += 1


        cc0 = 'chan num: %d/%.2f' % (len(self.chans), len(self.chans)/3.0) + "\n"
        cclen += len(cc0)
        ccs.append(cc0)

        resp.setHeader('Content-Length', '%d' % cclen)
        resp.writeHead(200)

        for cc0 in ccs: resp.write(cc0)
        
        resp.end()

        return

    # promise原理的优雅关闭与清理
    def _toxchanPromiseResults(self, chan):

        chan.peer_close = chan.rudp.peer_closed
        promise_results = {
            'peer_close': chan.peer_close,
            'sock_close': chan.sock_close,
            'rudp_close': chan.rudp_close,
        }
        
        nowtime = QDateTime.currentDateTime()
        if chan.rudp.begin_close_time is not None:
            qDebug(str(chan.rudp.begin_close_time.msecsTo(nowtime)))
        if chan.rudp.self_passive_close is False:
            promise_results['active_state'] = (chan.rudp.state == 'TIME_WAIT')
            if chan.rudp.begin_close_time is None:
                promise_results['time_wait_timeout'] = False                
            else:
                duration = chan.rudp.begin_close_time.msecsTo(nowtime)
                promise_results['time_wait_timeout'] = (duration > 15000)
        else:
            promise_results['pasv_state'] = (chan.rudp.state == 'CLOSED')

        if chan.rudp.state == 'SYN_SENT':
            promise_results['connect_timeout'] = (chan.rudp.connect_begin_time.msecsTo(nowtime) > 30000)

        promise_result = True
        for pk in promise_results: promise_result = promise_result and promise_results[pk]

        promise_results['whole'] = promise_result

        ### some raw status
        promise_results['pasv_close'] = chan.rudp.self_passive_close
        promise_results['state'] = chan.rudp.state
        promise_results['conn_btime'] = chan.rudp.connect_begin_time.toString()
        promise_results['conn_dtime'] = chan.rudp.connect_begin_time.msecsTo(nowtime)
        promise_results['can_close'] = chan.rudp.can_close

        promise_results['sock_rdlen'] = chan.rdlen
        promise_results['sock_wrlen'] = chan.wrlen
        
        return promise_results

    
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

    
    tuncli = ToxNetTunCli(config_file)
    tuncli.start()
    
    app.exec_()
    return

if __name__ == '__main__': main()


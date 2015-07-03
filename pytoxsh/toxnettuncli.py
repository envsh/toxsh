import sys,time
import json

from PyQt5.QtCore import *
from PyQt5.QtNetwork import *

import qtutil
from qtoxkit import *
from toxtunutils import *
from srudp import *


class ToxNetTunCli(QObject):
    def __init__(self, parent = None):
        super(ToxNetTunCli, self).__init__(parent)
        self.cfg = ToxTunConfig('./toxtun.ini')
        self.toxkit = None # QToxKit
        self.tcpsrvs = {}  # id => srv
        self.cons = {}     # peer => con
        self.chans = {}   # sock => chan, rudp => chan
        self.cmdno = 0
        
        return

    def start(self):
        self._startToxNet()
        self._startTcpServer()
        qDebug('started')
        return

    def _startToxNet(self):
        toxkit = QToxKit('toxcli', False)

        toxkit.connected.connect(self._toxnetConnected)
        toxkit.disconnected.connect(self._toxnetDisconnected)
        toxkit.friendAdded.connect(self._toxnetFriendAdded)
        toxkit.friendConnectionStatus.connect(self._toxnetFriendConnectionStatus)
        toxkit.friendConnected.connect(self._toxnetFriendConnected)
        toxkit.newMessage.connect(self._toxnetFriendMessage)
        
        self.toxkit = toxkit

        return

    def _startTcpServer(self):
        i = 0
        for rec in self.cfg.recs:
            srv = QTcpServer()
            srv.newConnection.connect(self._onNewTcpConnection)
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

            self.toxkit.friendAdd(con.peer, 'from toxtun cli %d' % i)
            i = i + 1
            
        return
    
    def _toxnetDisconnected(self):
        qDebug('here')
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

    def _toxnetFriendMessage(self, friendId, msg):
        qDebug(friendId)

        tmsg = json.JSONDecoder().decode(msg)
        qDebug(str(tmsg))
        
        # dispatch的过程
        opkt = SruPacket2.decode(msg)
        if opkt.msg_type == 'SYN2':
            jmsg = opkt.extra
            cmdnokey = 'cmdno_%d' % jmsg['cmdno']
            chan = self.chans[cmdnokey]
            chan.chano = jmsg['chano']
            self.chans[chan.chano] = chan
            self.chans.pop(cmdnokey)

            ropkt = chan.rudp.buf_recv_pkt(msg)
            self.toxkit.sendMessage(chan.con.peer, ropkt)

            pass
        elif opkt.msg_type == 'DATA':
            jmsg = opkt.extra
            chan = self.chans[jmsg['chano']]
            chan.rudp.buf_recv_pkt(msg)
            qDebug('here')
            pass
        elif opkt.msg_type == 'FIN1':
            jmsg = opkt.extra
            chan = self.chans[jmsg['chano']]
            ropkt = chan.rudp.buf_recv_pkt(msg)
            if ropkt is not None: self.toxkit.sendMessage(chan.con.peer, ropkt)
        else:
            jmsg = opkt.extra
            chan = self.chans[jmsg['chano']]
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
        cmdno = self._nextCmdno()
        chan.cmdno = cmdno

        if type(data) == bytes: data = QByteArray(data)
        msg = {'cmd': 'write', 'cmdno': cmdno, 'chano': chan.chano, 'data': data.toHex().data().decode('utf8'),}

        msg = json.JSONEncoder().encode(msg)

        self.toxkit.sendMessage(chan.con.peer, msg)
        return

    def _toxchanConnected(self):
        qDebug('here')
        udp = self.sender()
        chan = self.chans[udp]
        while chan.sock.bytesAvailable() > 0:
            bcc = chan.sock.read(128)
            print(bcc)
            #self._toxnetWrite(chan, bcc)
            data = QByteArray(bcc).toHex().data().decode('utf8')
            extra = {'chano': chan.chano}
            jspkt = udp.buf_send_pkt(data, extra)
            self.toxkit.sendMessage(chan.con.peer, jspkt)
        
        return

    def _toxchanDisconnected(self):
        qDebug('here')
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

    
    def _nextCmdno(self):
        self.cmdno = self.cmdno +1
        return self.cmdno
    
    def _onNewTcpConnection(self):
        srv = self.sender()
        rec = self.tcpsrvs[srv]

        sock = srv.nextPendingConnection()
        sock.readyRead.connect(self._onTcpReadyRead)
        sock.disconnected.connect(self._onTcpDisconnected)

        con = self.cons[srv]
        chan = ToxChannel(con, sock)
        chan.host = rec.remote_host
        chan.port = rec.remote_port
        chan.cmdno = self._nextCmdno()
        self.chans['cmdno_%d' % chan.cmdno] = chan
        self.chans[sock] = chan

        udp = Srudp()
        chan.rudp = udp
        self.chans[udp] = chan
        udp.connected.connect(self._toxchanConnected, Qt.QueuedConnection)
        udp.disconnected.connect(self._toxchanDisconnected)
        udp.readyRead.connect(self._toxchanReadyRead, Qt.QueuedConnection)

        extra = {'cmdno': chan.cmdno, 'host': chan.host, 'port': chan.port}
        pkt = udp.mkcon(extra)
        
        self.toxkit.sendMessage(con.peer, pkt)
                
        return

    def _onTcpDisconnected(self):
        qDebug('here')
        sock = self.sender()
        if sock not in self.chans:
            qDebug('maybe already closed')
            return

        chan = self.chans[sock]
        chano = chan.chano
        qDebug(chan.debugInfo())
        
        if chano not in self.chans:
            qDebug('maybe already closed222')
            self.chans.pop(sock)
            return
        
        cmdno = self._nextCmdno()
        extra = {'cmd': 'close', 'chano': chan.chano, 'cmdno': cmdno,}
        jspkt = chan.rudp.mkdiscon(extra)
        self.toxkit.sendMessage(chan.con.peer, jspkt)

        return
    
    def _onTcpReadyRead(self):
        qDebug('here')
        sock = self.sender()
        chan = self.chans[sock]

        qDebug(str(chan.chano))
        if chan.chano == 0: return

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
        rawdata = QByteArray.fromHex(data)

        n = sock.write(rawdata)
        chan.wrlen += n
        qDebug('XDR: toxnet->sock: %d/%d' % (n, chan.wrlen))
        
        return
    
def main():
    app = QCoreApplication(sys.argv)
    qtutil.pyctrl()

    tuncli = ToxNetTunCli()
    tuncli.start()
    
    app.exec_()
    return

if __name__ == '__main__': main()


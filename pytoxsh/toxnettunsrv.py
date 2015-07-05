import sys,time
import json

from PyQt5.QtCore import *
from PyQt5.QtNetwork import *

import qtutil
from qtoxkit import *
#from toxsocket import *
from toxtunutils import *
from srudp import *


class ToxNetTunSrv(QObject):
    def __init__(self, parent = None):
        super(ToxNetTunSrv, self).__init__(parent)
        self.toxkit = None # QToxKit
        self.cons = {}  # peer => con
        self.chans = {} # sock => chan, toxsock => chan, rudp => chan
        #self.host = '127.0.0.1'
        #self.port = 80
        self.chano = 0
        self.cmdno = 0
        
        return

    def start(self):
        toxkit = QToxKit('anon', True)

        toxkit.connected.connect(self._toxnetConnected)
        toxkit.disconnected.connect(self._toxnetDisconnected)
        toxkit.friendAdded.connect(self._toxnetFriendAdded)
        toxkit.friendConnectionStatus.connect(self._toxnetFriendConnectionStatus)
        toxkit.friendConnected.connect(self._toxnetFriendConnected)
        toxkit.newMessage.connect(self._toxnetFriendMessage)
        
        self.toxkit = toxkit
        
        return

    def _toxnetConnected(self):
        qDebug('here')
        toxsock = self.sender()
        #ses = self.cons[toxsock]
        #ses.tox_connected = True

        #ses.sock.readyRead.emit()
        return
    
    def _toxnetDisconnected(self):
        qDebug('here')
        return

    def _toxnetFriendAdded(self, fid):
        qDebug('hehe:' + fid)
        #con = self.toxcons[fid]

        con = ToxConnection()
        con.peer = fid

        self.cons[fid] = con
        
        return

    def _toxnetFriendConnectionStatus(self, fid, status):

        return
    
    def _toxnetFriendConnected(self, fid):
        qDebug('herhe:' + fid)
        # 只重启动srv端时，有可能好友还在线。
        if fid not in self.cons:
            con = ToxConnection()
            con.peer = fid
            self.cons[fid] = con
        
        return

    def _nextChano(self):
        self.chano = self.chano +1
        return self.chano

    def _nextCmdno(self):
        self.cmdno = self.cmdno +1
        return self.cmdno
    
    def _toxnetFriendMessage(self, friendId, msg):
        qDebug(friendId)
        con = self.cons[friendId]

        tmsg = json.JSONDecoder().decode(msg)
        qDebug(str(tmsg))
        
        opkt = SruPacket2.decode(msg)
        if opkt.msg_type == 'SYN':
            jmsg = opkt.extra
            host = jmsg['host']
            port = jmsg['port']
            
            sock = QTcpSocket()
            sock.setReadBufferSize(567)
            sock.connected.connect(self._onTcpConnected)
            sock.disconnected.connect(self._onTcpDisconnected)
            sock.readyRead.connect(self._onTcpReadyRead)
            sock.error.connect(self._onTcpError)
            sock.connectToHost(QHostAddress(host), port)

            chan = ToxChannel(con, sock)
            chan.host = host
            chan.port = port
            chan.chano = self._nextChano()
            chan.cmdno = jmsg['cmdno']

            self.chans[sock] = chan
            self.chans[chan.chano] = chan

            transport = ToxTunTransport(self.toxkit, chan.con.peer)
            chan.transport = transport
            
            udp = Srudp()
            chan.rudp = udp
            self.chans[udp] = chan
            udp.setTransport(transport)
            udp.readyRead.connect(self._toxchanReadyRead, Qt.QueuedConnection)
            udp.bytesWritten.connect(self._toxchanBytesWritten, Qt.QueuedConnection)
            udp.disconnected.connect(self._toxchanDisconnected, Qt.QueuedConnection)

            extra = {'chano': chan.chano}
            res = udp.buf_recv_pkt(msg, extra)
            # ropkt = udp.buf_recv_pkt(msg)
            # ropkt = SruPacket2.decode(ropkt)
            # ropkt.extra['chano'] = chan.chano
            # ropkt = ropkt.encode()
            # self.toxkit.sendMessage(chan.con.peer, ropkt)
            
            pass
        elif opkt.msg_type == 'FIN1':
            jmsg = opkt.extra
            chan = self.chans[jmsg['chano']]
            res = chan.rudp.buf_recv_pkt(msg)
            # ropkt = chan.rudp.buf_recv_pkt(msg)
            # self.toxkit.sendMessage(chan.con.peer, ropkt)
        elif opkt.msg_type == 'DATA':
            jmsg = opkt.extra
            chan = self.chans[jmsg['chano']]
            res = chan.rudp.buf_recv_pkt(msg)
            # jspkt = chan.rudp.buf_recv_pkt(msg)
            # self.toxkit.sendMessage(chan.con.peer, jspkt)
        else:
            jmsg = opkt.extra
            chan = self.chans[jmsg['chano']]
            res = chan.rudp.buf_recv_pkt(msg)
            
                    
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
        
        return

    # @param data bytes | QByteArray
    def _toxnetWrite(self, chan, data):
        cmdno = self._nextCmdno()
        chan.cmdno = cmdno

        extra = {'chano': chan.chano}
        res = chan.rudp.buf_send_pkt(data, extra)
        # if type(data) == bytes: data = QByteArray(data)

        # extra = {'chano': chan.chano}
        # data = data.toHex().data().decode('utf8')
        # opkt = chan.rudp.buf_send_pkt(data, extra)

        # msg = opkt
        # self.toxkit.sendMessage(chan.con.peer, msg)
        
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

    def _toxchanBytesWritten(self):
        udp = self.sender()
        chan =self.chans[udp]

        chan.sock.readyRead.emit()
        
        return

    def _toxchanDisconnected(self):
        qDebug('here')
        return

    def _onTcpConnected(self):
        qDebug('here')

        sock = self.sender()
        chan = self.chans[sock]

        reply = {'cmd': 'connect', 'res': True, 'chano': chan.chano, 'cmdno': chan.cmdno, }
        #    self.toxkit.sendMessage(chan.con.peer, reply)
        return
            
    def _onTcpDisconnected(self):
        qDebug('here %d' % len(self.chans))
        sock = self.sender()

        if sock not in self.chans:
            qDebug('maybe already closed123')
            return

        chan = self.chans[sock]
        chano = chan.chano

        if chano not in self.chans:
            qDebug('maybe already closed222')
            self.chans.pop(sock)
            return

        cmdno = self._nextCmdno()
        msg = {'cmd': 'close', 'chano': chan.chano, 'cmdno': cmdno, }

        extra = msg
        #jspkt = chan.rudp.mkdiscon(extra)
        #self.toxkit.sendMessage(chan.con.peer, jspkt)

        return

    def _onTcpError(self, error):
        sock = self.sender()
        qDebug('herhe %s' %  sock.errorString())
        chan = self.chans[sock]
        chano = chan.chano

        if chano not in self.chans:
            qDebug('maybe already closed333')
            self.chans.pop(sock)
            return

        cmdno = self._nextCmdno()
        msg = {'cmd': 'close', 'chano': chan.chano, 'cmdno': cmdno,}
        
        extra = msg
        # jspkt = chan.rudp.mkdiscon(extra)
        # self.toxkit.sendMessage(chan.con.peer, jspkt)

        return
    
    def _onTcpReadyRead(self):
        qDebug('here')
        sock = self.sender()
        chan = self.chans[sock]

        extra = {'chano': chan.chano}
        cnter = 0
        tlen = 0
        while sock.bytesAvailable() > 0:
            #bcc = sock.read(128)
            #self._toxnetWrite(chan, bcc)
            bcc = sock.peek(128)
            encbcc = chan.transport.encodeData(bcc)
            res = chan.rudp.attemp_send(encbcc, extra)
            if res is True:
                bcc1 = sock.read(128)
                chan.rdlen += len(bcc)
                cnter += 1
                tlen += len(bcc)
            else: break
                
        qDebug('XDR: sock->toxnet: %d/%d, %d' % (tlen, chan.rdlen, cnter))
        return

    
    def _tcpWrite(self, chan, data):
        qDebug('hrehe')
        sock = chan.sock

        rawdata = QByteArray.fromHex(data)
        print(rawdata)
        n = sock.write(rawdata)
        chan.wrlen += n
        qDebug('XDR: toxnet->sock: %d/%d' % (n, chan.wrlen))
        
        return


def main():
    app = QCoreApplication(sys.argv)
    qtutil.pyctrl()

    tunsrv = ToxNetTunSrv()
    tunsrv.start()
    
    app.exec_()
    return


if __name__ == '__main__': main()


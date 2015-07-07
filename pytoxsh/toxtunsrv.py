import sys,time
import json

from PyQt5.QtCore import *
from PyQt5.QtNetwork import *

import qtutil
from qtoxkit import *
# from toxsocket import *
from toxtunutils import *
# from srudp import *

class ToxTunSrv(QObject):
    def __init__(self, parent = None):
        super(ToxTunSrv, self).__init__(parent)
        return

class ToxTunFileSrv(ToxTunSrv):
    def __init__(self, parent = None):
        super(ToxTunFileSrv, self).__init__(parent)
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
        toxkit.fileRecv.connect(self._toxnetFileRecv, Qt.QueuedConnection)
        toxkit.fileRecvChunk.connect(self._toxnetFileRecvChunk, Qt.QueuedConnection)
        toxkit.fileChunkRequest.connect(self._toxnetFileChunkRequest, Qt.QueuedConnection)
        
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

    def _toxnetFileRecv(self, friend_id, file_number, file_size, file_name):
        qDebug(file_name)

        # con = self.cons[friend_id]
        con = None
        for id in self.cons:
            if id[0:64] == friend_id:
                con = self.cons[id]
                break

        if con is None: qDebug('warning con not found')
            
        segs = file_name.split('_')
        qDebug(str(segs))

        cmdno = int(segs[1])
        host = segs[2]
        port = int(segs[3])
        
        sock = QTcpSocket()
        sock.setReadBufferSize(1234)
        sock.connected.connect(self._onTcpConnected)
        sock.disconnected.connect(self._onTcpDisconnected)
        sock.readyRead.connect(self._onTcpReadyRead)
        sock.error.connect(self._onTcpError)
        sock.connectToHost(QHostAddress(host), port)
        
        chan = ToxTunFileChannel(con, sock)
        chan.host = host
        chan.port = port
        chan.chano = self._nextChano()
        chan.cmdno = cmdno
        chan.peer_file_number = file_number
        
        self.chans[sock] = chan
        self.chans[chan.chano] = chan
        self.chans[file_name] = chan

        srv_file_name = 'toxtunfilesrv_%d_%s_%d_toxfilesock' % (chan.cmdno, chan.host, chan.port)
        srv_file_size = pow(2, 56)
        srv_file_number = self.toxkit.fileSend(con.peer, srv_file_size, srv_file_name)
        chan.self_file_number = srv_file_number
        qDebug('new cli file %d' % srv_file_number)
        
        self.toxkit.fileControl(friend_id, file_number, 0)

        return

    def _toxnetFileRecvChunk(self, friend_id, file_number, position, length):
        qDebug('here')
        return

    def _toxnetFileChunkRequest(self, friend_id, file_number, position, length):
        qDebug('here')
        return
    
    def _toxnetFriendMessage(self, friendId, msg):
        qDebug(friendId)
        
        return
    
    # @param data bytes | QByteArray
    def _toxnetWrite(self, chan, data):
        cmdno = self._nextCmdno()
        chan.cmdno = cmdno

        extra = {'chano': chan.chano}
        res = chan.rudp.buf_send_pkt(data, extra)
        
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

    def _toxchanCanClose(self):
        udp = self.sender()
        chan = self.chans[udp]

        cmdno = self._nextCmdno()
        extra = {'cmd': 'close', 'chano': chan.chano, 'cmdno': cmdno, }
        res = chan.rudp.mkdiscon(extra)
        
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
        chan.transport.closed = True
        chan.rudp.startCheckClose()
        
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
        chan.transport.closed = True
        # chan.rudp.startCheckClose()
        # jspkt = chan.rudp.mkdiscon(extra)
        # self.toxkit.sendMessage(chan.con.peer, jspkt)

        return
    
    def _onTcpReadyRead(self):
        qDebug('here')
        sock = self.sender()
        chan = self.chans[sock]

        peekSize = 897  # 987时就有可能导致超长拆包发送
        extra = {'chano': chan.chano}
        cnter = 0
        tlen = 0
        # while sock.bytesAvailable() > 0:
        #     bcc = sock.peek(peekSize)
        #     res = chan.rudp.attemp_send(bcc, extra)
        #     if res is True:
        #         bcc1 = sock.read(len(bcc))
        #         chan.rdlen += len(bcc)
        #         cnter += 1
        #         tlen += len(bcc)
        #     else: break
                
        qDebug('XDR: sock->toxnet: %d/%d, %d' % (tlen, chan.rdlen, cnter))
        return

    
    def _tcpWrite(self, chan, data):
        qDebug('hrehe')
        sock = chan.sock

        # rawdata = QByteArray.fromHex(data)
        rawdata = chan.transport.decodeData(data)
        print(rawdata)
        n = sock.write(rawdata)
        chan.wrlen += n
        qDebug('XDR: toxnet->sock: %d/%d' % (n, chan.wrlen))
        
        return


def main():
    app = QCoreApplication(sys.argv)
    qtutil.pyctrl()

    tunsrv = ToxTunFileSrv()
    tunsrv.start()
    
    app.exec_()
    return


if __name__ == '__main__': main()


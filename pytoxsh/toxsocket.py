
import sys,time

from PyQt5.QtCore import *
from PyQt5.QtNetwork import *

from qtoxkit import *

_srvpeer = '6BA28AC06C1D57783FE017FA9322D0B356E61404C92155A04F64F3B19C75633E8BDDEFFA4856'
class ToxConnection():
    def __init__(self):
        self.host = ''
        self.port = ''
        self.c2s_sock = ''
        self.s2c_sock = ''
        self.peer = None
        #self.peer = _srvpeer
        #self.toxkit = None
        
        return

class ToxChannel():
    def __init__(self):

        return

_toxkit = None
def _toxkit_inst():
    if _toxkit is None: _toxkit = QToxKit()
    return _toxkit

# 一个ToxSocket代表着。。。
class ToxSocket(QObject):
    connected = pyqtSignal()
    disconnected = pyqtSignal()
    readyRead = pyqtSignal('QString')

    def __init__(self, parent = None):
        super(ToxSocket, self).__init__(parent)
        self.msgs = {}

        self.toxkit = None # QToxKit
        self.has_pending_onnect = False
        self.toxcon = ToxConnection()
        self.toxcons = {}   # peer => ToxConnection

        self.con_file_name = 'toxnet_socket_over_toxfile_%s_%d.vrt'
        self.con_file_name = 'toxsockfile_client_%s_%d.vrt'
        #self.conn_file_name = 'toxsockfile_server_%s_%d.vrt'

        #self.xinit()
        return

    def xinit(self):
        # 多toxcli实例模式，不存储
        toxkit = QToxKit('toxcli', False)
        toxkit.connected.connect(self._toxnetConnected)
        toxkit.disconnected.connect(self._toxnetDisconnected)
        toxkit.friendAdded.connect(self._toxnetFriendAdded)
        toxkit.friendConnected.connect(self._toxnetFriendConnected)
        toxkit.newMessage.connect(self._toxnetFriendMessage)
        self.toxkit = toxkit

        return
    def connectToHost(self, host, port, peer):

        con = ToxConnection()
        con.host = host
        con.port = port
        con.peer = peer

        self.toxcons[peer] = con

        reqmsg = 'from toxtun node (%s:%d)' % (host, port)
        self.toxkit.friendAdd(peer, reqmsg)

        return

    # server模式调用
    def setFD(self, toxkit, clipeer):
        
        toxkit.connected.connect(self._toxnetConnected)
        toxkit.disconnected.connect(self._toxnetDisconnected)
        toxkit.friendConnected.connect(self._toxnetFriendConnected)
        toxkit.newMessage.connect(self._toxnetFriendMessage)

        con = ToxConnection()
        #con.host = tohost
        #con.port = toport
        con.peer = clipeer
        self.toxcons[clipeer] = con

        return

    def _toxnetFriendAdded(self, fid):
        qDebug('hehe:' + fid)
        con = self.toxcons[fid]
        
        return
    def _toxnetFriendConnected(self, fid):
        qDebug('herhe:' + fid)
        qDebug('herhe:' + self.pc_host)
        #if fid == self.pc_host[0:len(fid)]:
        #    self.has_pending_onnect = False
        #    self.connectToHost(self.pc_host, self.pc_port)
        self.connected.emit()
        
        return

    def _toxnetConnected(self):
        qDebug('herhe')
        #self.has_pending_onnect = False

        #self.toxcon.toxkit.friendAdd(_srvpeer, 'from tox cli')
        
        return
    def _toxnetDisconnected(self):
        qDebug('here')
        return

    def _toxnetFriendMessage(self, friendId, msg):
        qDebug(friendId)

        # dispatch的过程
        if friendId in self.toxcons:
            if friendId not in self.msgs: self.msgs[friendId] = []
            self.msgs[friendId].append(msgs)
            self.readyRead.emit(friendId)
        #有可能产生消息积压，如果调用端不读取的话
        return
    
    def _tcpReadyRead(self):
        # no use
        sock = self.sender()
        bcc = sock.readAll()

        toxkit = self.toxcon.toxkit
        
        return
    
    def read(self, peer):
        if peer in self.msgs:
            msg = self.msgs[peer].pop()
            return msg
        return None
    def readAll(self):
        return

    def write(self, data):
        toxkit = self.toxcon.toxkit
        toxkit.sendMessage(self.toxcon.peer, data)
        return
    
class ToxFileSocket(ToxSocket):
    
    def __init__(self, parent = None):
        super(ToxFileSocket, self).__init__(parent)

        self.tox.fileRecvControl.connect(self._toxnetFileControl)
        self.tox.fileRecv.connect(self._toxnetFileRecv)
        
        return
    
    def connectToHost(self, host, port):
        qDebug('herhe')
        fsize = 1024 * 1024 * 1024 * 1024   # 1T stream
        if self.tox.isConnected():
            file_name = self.conn_file_name % (host[0:5], port)
            self.tox.sendFile(host, fsize, file_name)
        else:
            self.has_pending_onnect = True
            self.pc_host = host
            self.pc_port = port
            
        return

    def _toxnetFriendConnected(self, fid):
        qDebug('herhe:' + fid)
        qDebug('herhe:' + self.pc_host)
        if fid == self.pc_host[0:len(fid)]:
            self.has_pending_onnect = False
            self.connectToHost(self.pc_host, self.pc_port)

        return

    def _toxnetFileControl(self, friend_pubkey, file_name, control):
        qDebug('herhe:' + friend_pubkey)
        if control == 0: # ok
            self.toxconn.c2s_sock = file_name
            self.connected.emit()
            qDebug('real connected')
            pass
        elif control == 1:
            pass
        elif control == 2:
            pass
        else:
            pass
        return

    def _toxnetFileRecv(self, friend_pubkey, file_number, file_size, filename):
        qDebug('herhe')
        qDebug(friend_pubkey)
        qDebug(str(file_number))
        qDebug(str(file_size))
        qDebug(filename)

        self.tox.fileControl(friend_pubkey, file_number, 0)
        
        return
    
    def _toxnetConnected(self):
        qDebug('herhe')
        #self.has_pending_onnect = False
        #self.connectToHost(self.pc_host, self.pc_port)
        
        return
    def _toxnetDisconnected(self):
        return


class _ToxConnection2():
    def __init__(self):
        self.host = ''
        self.port = ''
        self.sock = None
        self.c2s_sock = ''
        self.s2c_sock = ''
        self.peer = '6BA28AC06C1D57783FE017FA9322D0B356E61404C92155A04F64F3B19C75633E8BDDEFFA4856'
        
        return

import json
class _ToxTcpPacket():
    def __init__(self):
        self.conno = 0
        self.pktno = 0
        self.cmd = ''  # connect/disconnect
        self.args = []  # array like
        return
    
    def get(self):

        return
    def set(self, pkt):
        return
    
class ToxMsgSocket(QObject):
    connected = pyqtSignal()
    disconnected = pyqtSignal()
    readyRead = pyqtSignal()

    def __init__(self, parent = None):
        super(ToxMsgSocket, self).__init__(parent)
        self.tox = QToxKit()
        self.tox.connected.connect(self._toxnetConnected)
        self.tox.disconnected.connect(self._toxnetDisconnected)
        self.tox.friendConnected.connect(self._toxnetFriendConnected)

        self.has_pending_onnect = False
        self.pc_host = ''
        self.pc_port = 0
        self.toxcon = _ToxConnection2()

        self.conn_file_name = 'toxnet_socket_over_toxfile_%s_%d.vrt'
        self.conn_file_name = 'toxsockfile_client_%s_%d.vrt'
        #self.conn_file_name = 'toxsockfile_server_%s_%d.vrt'

        return

    # send cmd to tox server peer
    def connectToHost(self, host, port):
        
        # sock = QTcpSocket()
        # self.toxcon.sock = sock
        
        # sock.connected.connect(self._tcpConnected)
        # sock.disconnected.connect(self._tcpClosed)
        # sock.readyRead.connect(self._tcpReadyRead)

        # sock.connectToHost(host, port)

        return

    def _tcpConnected(self):
        sock = self.sender()
        self.connected.emit()
        return
    def _tcpClosed(self):
        sock = self.sender()
        self.disconnected.emit()
        return
    
    def _tcpReadyRead(self):
        return
    
    def _toxnetFriendConnected(self, fid):
        qDebug('herhe:' + fid)
        qDebug('herhe:' + self.pc_host)
        if fid == self.pc_host[0:len(fid)]:
            self.has_pending_onnect = False
            self.connectToHost(self.pc_host, self.pc_port)

        return

    def _toxnetConnected(self):
        qDebug('herhe')
        #self.has_pending_onnect = False
        #self.connectToHost(self.pc_host, self.pc_port)
        
        return
    def _toxnetDisconnected(self):
        return

class _TestToxMsgSocket(QObject):
    def __init__(self, parent = None):
        super(_TestToxMsgSocket, self).__init__(parent)
        self.toxsock = None
        return

    def runit(self):
        self.toxsock = ToxMsgSocket()
        self.toxsock.connected.connect(self.onConnected)
        self.toxsock.connectToHost('127.0.0.1', 80)

        return
    def onConnected(self):
        qDebug('here')
        
        return

class _ToxConnection3():
    def __init__(self):
        self.host = ''
        self.port = 0
        self.sock = QTcpSocket()
        self.sock = None
        self.c2s_sock = ''
        self.s2c_sock = ''
        self.peer = ''
        self.toxkit = None
        
        return

    def assign(self, other):
        self.host = other.host
        self.port = other.port
        self.sock = other.sock
        self.peer = other.peer
        self.toxkit = other.toxkit
        return


class ToxServerRouter(QObject):
    def __init__(self, parent = None):
        
        return
    
# ToxServer的作用说明
# 是一个被连接端，有固定的toxid
# 它收到被连接（加好友）事件，把这个连接转发到本地真实的tcp server上
# ToxServer内部使用的是QTcpSocket客户端
# 虽然有listen方法，但不会真的listen某个本地端口，listen的端口只是真正的后端tcp server端口
# ToxServer的listen方法要做的是启动QToxKit实例，并正确连接到toxnet上。
class ToxServer(QObject):
    newConnection = pyqtSignal()
    
    def __init__(self, parent = None):
        super(ToxServer, self).__init__(parent)
        self.toxkit = None # QToxKit
        #self.srv = QTcpServer()
        #self.srv = None
        self.toxcon = _ToxConnection3()
        self.cons = {}
        self.newcons = []
        
        return

    # 与QTcpServer不同的是，这里的host可以是本地ip，也可以是远程ip
    def listen(self, host, port):
        self.toxcon.host = host
        self.toxcon.port = port

        toxkit = QToxKit('anon', True)
        toxkit.connected.connect(self._toxnetConnected)
        toxkit.disconnected.connect(self._toxnetDisconnected)
        toxkit.friendAdded.connect(self._onFriendAdded)
        toxkit.newMessage.connect(self._onNewMessage)
        self.toxcon.toxkit = toxkit
        self.toxkit = toxkit
        
        return

    def _toxnetConnected(self):
        qDebug('herhe')
        toxkit = self.toxcon.toxkit
        # toxkit.friendAdd(self.toxcon.peer, 'preconnect')

        return
    def _toxnetDisconnected(self):
        qDebug('here')
        return

    def _onFriendAdded(self, friendId):
        qDebug(friendId)
        
        toxcon = _ToxConnection3()
        toxcon.assign(self.toxcon)
        toxcon.peer = friendId

        self.cons[friendId] = toxcon
        self.newcons.append(toxcon)

        sock = QTcpSocket()
        toxcon.sock = sock
        sock.connectToHost(QHostAddress(self.toxcon.host), self.toxcon.port)
        sock.connected.connect(self._onTcpConnected)

        self.cons[sock] = toxcon

        self._onNewToxConnection()

        return

    def _onTcpConnected(self):
        qDebug('here')
        sock = self.sender()
        toxcon = self.cons[sock]
        
        return
    
    def _onTcpReadyRead(self):
        qDebug('here')
        sock = self.sender()
        toxcon = self.cons[sock]

        nbytes = sock.bytesAvailable()
        qDebug(str(nbytes))
        
        return

    def _onNewMessage(self, friendId, msg):
        qDebug('here')
        
        toxcon = self.cons[friendId]
        sock = toxcon.sock

        # sock.write(msg)
        
        return
    
    def _onNewToxConnection(self):
        self.newConnection.emit()
        return

    def nextPendingConnection(self):
        toxcon = self.newcons.pop()

        toxsock = ToxSocket()
        toxsock.setFD(self.toxkit, toxcon.peer)
        toxsock.readyRead.connect(self._onToxNetReadyRead)

        self.cons[toxsock] = toxcon
        return toxsock

    def _onToxNetReadyRead(self, peer):
        qDebug(peer)
        toxsock = self.sender()
        con = self.cons[toxsock]

        bcc = toxsock.read(peer)
        qDebug(str(len(bcc)))
        
        return

class _TestToxServer(QObject):
    def __init__(self):
        super(_TestToxServer, self).__init__()
        self.srv = None
        return

    def runit(self):
        srv = ToxServer()
        self.srv = srv
        srv.newConnection.connect(self._onNewConnection)
        srv.listen('127.0.0.1', 5678)
        
        return

    def _onNewConnection(self):
        sock = self.srv.nextPendingConnection()  # ToxSock object
        qDebug('here' + str(sock))

        sock.write("hhohohoho")
        return

def main():
    import qtutil

    app = QCoreApplication(sys.argv)
    qtutil.pyctrl()

    fid = '398C8161D038FD328A573FFAA0F5FAAF7FFDE5E8B4350E7D15E6AFD0B993FC529FA90C343627'
    #sock = ToxFileSocket()
    #sock.connectToHost(fid, 12345)
    #tster = _TestToxMsgSocket()
    #tster.runit()
    tster = _TestToxServer()
    tster.runit()

    app.exec_()
    return

if __name__ == '__main__': main()
    


import sys,time

from PyQt5.QtCore import *

from qtoxkit import *

class ToxConnection():
    def __init__(self):
        self.host = ''
        self.port = ''
        self.c2s_sock = ''
        self.s2c_sock = ''
        
        return
    
class ToxSocket(QObject):

    connected = pyqtSignal()
    disconnected = pyqtSignal()
    
    def __init__(self, parent = None):
        super(ToxSocket, self).__init__(parent)

        self.tox = QToxKit()
        self.tox.connected.connect(self._toxnetConnected)
        self.tox.disconnected.connect(self._toxnetDisconnected)
        self.tox.friendConnected.connect(self._toxnetFriendConnected)
        self.tox.fileRecvControl.connect(self._toxnetFileControl)
        self.tox.fileRecv.connect(self._toxnetFileRecv)
        
        self.has_pending_onnect = False
        self.pc_host = ''
        self.pc_port = 0
        self.toxconn = ToxConnection()

        self.conn_file_name = 'toxnet_socket_over_toxfile_%s_%d.vrt'
        self.conn_file_name = 'toxsockfile_client_%s_%d.vrt'
        #self.conn_file_name = 'toxsockfile_server_%s_%d.vrt'
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

    
class ToxServer(QObject):
    def __init__(self, parent = None):
        super(ToxServer, self).__init__(parent)

        return

    # 与QTcpServer不同的是，这里的host可以是本地ip，也可以是远程ip
    def listen(self, host, port):
        return



def main():
    import qtutil

    app = QCoreApplication(sys.argv)
    qtutil.pyctrl()

    fid = '398C8161D038FD328A573FFAA0F5FAAF7FFDE5E8B4350E7D15E6AFD0B993FC529FA90C343627'
    sock = ToxSocket()
    sock.connectToHost(fid, 12345)

    app.exec_()
    return

if __name__ == '__main__': main()
    

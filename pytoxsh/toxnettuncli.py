import sys,time

from PyQt5.QtCore import *
from PyQt5.QtNetwork import *

import qtutil 
from toxsocket import *

class _TunSession():
    def __init__(self):
        self.sock = QTcpSocket()
        self.sock = None
        self.toxsock = None
        self.tox_connected = False
        return

class ToxNetTunCli(QObject):
    def __init__(self, parent = None):
        super(ToxNetTunCli, self).__init__(parent)
        self.cons = {}
        self.srv = QTcpServer()
        self.srv.newConnection.connect(self._onNewConnection)
        
        return

    def start(self):
        self.srv.listen(QHostAddress.Any, 5678)
        qDebug('started')
        return

    def _onNewConnection(self):
        sock = self.srv.nextPendingConnection()
        ses = _TunSession()
        ses.sock = sock

        sock.readyRead.connect(self._onTcpReadyRead)
        sock.disconnected.connect(self._onTcpDisconnected)

        toxsock = ToxSocket()
        ses.toxsock = toxsock
        toxsock.connected.connect(self._onToxConnected)
        toxsock.disconnected.connect(self._onToxDisconnected)
        toxsock.readyRead.connect(self._onToxReadyReady)
        toxsock.connectToHost('127.0.0.1', 80)

        self.cons[sock] = ses
        self.cons[toxsock] = ses
        
        return

    def _onToxConnected(self):
        qDebug('here')
        toxsock = self.sender()
        ses = self.cons[toxsock]
        ses.tox_connected = True

        ses.sock.readyRead.emit()
        return
    
    def _onToxDisconnected(self):
        qDebug('here')
        return

    def _onToxReadyReady(self):
        qDebug('here')
        toxsock = self.sender()
        ses = self.cons[toxsock]
        sock = ses.sock

        bcc = toxsock.read()
        sock.write(bcc)
        
        return
    
    def _onTcpDisconnected(self):
        qDebug('here')
        return
    
    def _onTcpReadyRead(self):
        qDebug('here')
        sock = self.sender()
        ses = self.cons[sock]
        toxsock = ses.toxsock
        
        if ses.tox_connected is False:
            qDebug('not connected, omit')
            return

        bcc = sock.readAll()
        toxsock.write(bcc)

        return


def main():
    app = QCoreApplication(sys.argv)
    qtutil.pyctrl()

    tuncli = ToxNetTunCli()
    tuncli.start()
    
    app.exec_()
    return

if __name__ == '__main__': main()


import sys,time

from PyQt5.QtCore import *
from PyQt5.QtNetwork import *

import qtutil 
from toxsocket import *


class ToxNetTunSrv(QObject):
    def __init__(self, parent = None):
        super(ToxNetTunSrv, self).__init__(parent)
        self.toxsrv = None
        self.cons = {}  # sock => toxsock, toxsock => sock
        self.host = '127.0.0.1'
        self.port = 80
        
        return

    def start(self):
        toxsrv = ToxServer()
        toxsrv.newConnection.connect(self._onNewConnection)
        self.toxsrv = toxsrv
        toxsrv.listen('127.0.0.1', 80)
        
        return

    def _onNewConnection(self):
        qDebug('here')
        toxsock = self.toxsrv.nextPendingConnection()
        toxsock.disconnected.connect(self._onToxDisconnected)
        toxsock.readyRead.connect(self._onToxReadyReady)
        
        sock = QTcpSocket()
        sock.connected.connect(self._onTcpConnected)
        sock.disconnected.connect(self._onTcpDisconnected)
        sock.readyRead.connect(self._onTcpReadyRead)

        sock.connectToHost(QHostAddress(self.host), self.port)

        self.cons[sock] = toxsock
        self.cons[toxsock] = sock
        return

    def _onToxConnected(self):
        qDebug('here')
        return
    
    def _onToxDisconnected(self):
        qDebug('here')
        return

    def _onToxReadyReady(self):
        qDebug('here')
        toxsock = self.sender()
        sock = self.cons[toxsock]

        bcc = toxsock.read()
        sock.write(bcc)
        return

    def _onTcpConnected(self):
        qDebug('here')
        return
            
    def _onTcpDisconnected(self):
        qDebug('here')
        return
    
    def _onTcpReadyRead(self):
        qDebug('here')
        sock = self.sender()
        toxsock = self.cons[sock]

        bcc = sock.readAll()
        #toxsock.write(bcc[0:100])
        toxsock.write(bcc)
        
        return


def main():
    app = QCoreApplication(sys.argv)
    qtutil.pyctrl()

    tunsrv = ToxNetTunSrv()
    tunsrv.start()
    
    app.exec_()
    return

if __name__ == '__main__': main()


import sys,time
import json

from PyQt5.QtCore import *
from PyQt5.QtNetwork import *

import qtutil
from qtoxkit import *
# from toxsocket import *
from toxtunconfig import *

class ToxConnection():
    def __init__(self):
        self.srv = None # QTcpServer
        self.peer = None
        return

class ToxChannel():
    def __init__(self, con, sock):
        self.con = con
        self.sock = sock #
        self.host = ''
        self.port = 0
        self.chano = 0
        self.cmdno = 0
        self.rdlen = 0
        self.wrlen = 0
        return

class ToxNetTunCli(QObject):
    def __init__(self, parent = None):
        super(ToxNetTunCli, self).__init__(parent)
        self.cfg = ToxTunConfig('./toxtun.ini')
        self.toxkit = None # QToxKit
        self.tcpsrvs = {}  # id => srv
        self.cons = {}
        self.chans = {}
        self.cmdno = 0
        return

    def start(self):
        self._startToxNet()
        self._startTcpServer()
        qDebug('started')
        return

    def _startToxNet(self):
        toxkit = QToxKit('toxcli', False)
        #toxsock.connected.connect(self._onToxConnected)
        #toxsock.disconnected.connect(self._onToxDisconnected)
        #toxsock.readyRead.connect(self._onToxReadyReady)
        #toxsock.connectToHost('127.0.0.1', 80)

        toxkit.connected.connect(self._toxnetConnected)
        toxkit.disconnected.connect(self._toxnetDisconnected)
        toxkit.friendAdded.connect(self._toxnetFriendAdded)
        toxkit.friendConnected.connect(self._toxnetFriendConnected)
        toxkit.newMessage.connect(self._toxnetFriendMessage)
        
        self.toxkit = toxkit

        return

    def _startTcpServer(self):
        i = 0
        for rec in self.cfg.recs:
            srv = QTcpServer()
            srv.newConnection.connect(self._onNewConnection)
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
    def _toxnetFriendConnected(self, fid):
        qDebug('herhe:' + fid)
        
        return

    def _toxnetFriendMessage(self, friendId, msg):
        qDebug(friendId)

        # dispatch的过程
        jmsg = json.JSONDecoder().decode(msg)
        qDebug(str(jmsg))
        
        if jmsg['cmd'] == 'connect':
            cmdnokey = 'cmdno_%d' % jmsg['cmdno']
            chan = self.chans[cmdnokey]
            chan.chano = jmsg['chano']
            self.chans[chan.chano] = chan
            self.chans.pop(cmdnokey)

            while chan.sock.bytesAvailable() > 0:
                bcc = chan.sock.read(128)
                print(bcc)
                self._toxnetWrite(chan, bcc)
            pass

        if jmsg['cmd'] == 'write':
            chan = self.chans[jmsg['chano']]
            self._tcpWrite(chan, jmsg['data'])
            pass

        if jmsg['cmd'] == 'close':
            chano = jmsg['chano']
            if chano in self.chans:
                chan = self.chans[chano]
                self.chans.pop(chano)
                chan.sock.close()
            pass

        #有可能产生消息积压，如果调用端不读取的话
        return

    # @param data bytes | QByteArray
    def _toxnetWrite(self, chan, data):
        cmdno = self._nextCmdno()
        chan.cmdno = cmdno

        if type(data) == bytes: data = QByteArray(data)
        msg = {
            'cmd': 'write',
            'cmdno': cmdno,
            'chano': chan.chano,
            'data': data.toHex().data().decode('utf8'),
        }

        msg = json.JSONEncoder().encode(msg)
        self.toxkit.sendMessage(chan.con.peer, msg)
        
        return
    
    def _nextCmdno(self):
        self.cmdno = self.cmdno +1
        return self.cmdno
    
    def _onNewConnection(self):
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

        req = {
            'cmd': 'connect',
            'host': chan.host,
            'port': chan.port,
            'cmdno': chan.cmdno,
        }

        req = json.JSONEncoder().encode(req)
        
        self.toxkit.sendMessage(con.peer, req)
                
        return

    def _onTcpDisconnected(self):
        qDebug('here')
        sock = self.sender()
        if sock not in self.chans:
            qDebug('maybe already closed')
            return

        chan = self.chans[sock]
        chano = chan.chano

        if chano not in self.chans:
            qDebug('maybe already closed222')
            self.chans.pop(sock)
            return
        
        cmdno = self._nextCmdno()
        msg = {
            'cmd': 'close',
            'chano': chan.chano,
            'cmdno': cmdno,
        }
        
        msg = json.JSONEncoder().encode(msg)
        self.toxkit.sendMessage(chan.con.peer, msg)

        return
    
    def _onTcpReadyRead(self):
        qDebug('here')
        sock = self.sender()
        chan = self.chans[sock]

        qDebug(str(chan.chano))
        if chan.chano == 0: return

        while sock.bytesAvailable() > 0:
            bcc = chan.sock.read(128)
            print(bcc)
            self._toxnetWrite(chan, bcc)
            chan.rdlen += len(bcc)

        qDebug('XDR: sock->toxnet: %d' % chan.rdlen)
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


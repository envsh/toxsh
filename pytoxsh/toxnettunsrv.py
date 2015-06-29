import sys,time
import json

from PyQt5.QtCore import *
from PyQt5.QtNetwork import *

import qtutil
from qtoxkit import *
#from toxsocket import *
from toxtunutils import *


class ToxNetTunSrv(QObject):
    def __init__(self, parent = None):
        super(ToxNetTunSrv, self).__init__(parent)
        self.toxkit = None # QToxKit
        self.cons = {}  # peer => con
        self.chans = {} # sock => chan, toxsock => chan
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
        if status is True: self._toxnetOnlinePostHandler(fid)
        else: self._toxnetOfflinePostHandler(fid)
        return
    
    def _toxnetFriendConnected(self, fid):
        qDebug('herhe:' + fid)
        # 只重启动srv端时，有可能好友还在线。
        if fid not in self.cons:
            con = ToxConnection()
            con.peer = fid
            self.cons[fid] = con
        
        return

    def _toxnetOnlinePostHandler(self, peer):
        qDebug('here')
        # 找到所有有offline_buffers的chan,并重新发送消息
        for tpeer in self.chans:
            qDebug('%s==?%s' % (tpeer, peer))
            if tpeer != peer: continue
            chan = self.chans[tpeer]
            chan.offline_times[chan.offline_count].append(QDateTime.currentDateTime())
            self._toxnetResendOfflineMessages(chan)
            pass
        return

    def _toxnetOfflinePostHandler(self, peer):
        qDebug('here')
        # 找到所有chan,写入offline相关时间信息
        for tpeer in self.chans:
            qDebug('%s==?%s' % (tpeer, peer))
            if tpeer != peer: continue
            chan = self.chans[tpeer]
            chan.offline_count += 1
            chan.offline_times[chan.offline_count] = [QDateTime.currentDateTime()]
            pass
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
        
        # dispatch的过程
        jmsg = json.JSONDecoder().decode(msg)
        qDebug(str(jmsg))

        if jmsg['cmd'] == 'connect':
            host = jmsg['host']
            port = jmsg['port']
            
            sock = QTcpSocket()
            sock.connected.connect(self._onTcpConnected)
            sock.disconnected.connect(self._onTcpDisconnected)
            sock.readyRead.connect(self._onTcpReadyRead)
            sock.connectToHost(QHostAddress(host), port)

            chan = ToxChannel(con, sock)
            chan.host = host
            chan.port = port
            chan.chano = self._nextChano()
            chan.cmdno = jmsg['cmdno']

            self.chans[sock] = chan
            self.chans[chan.chano] = chan
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

        haspending = len(chan.offline_buffers) > 0
        status = self.toxkit.friendGetConnectionStatus(chan.con.peer)
        if status > 0 and haspending is False:
            self.toxkit.sendMessage(chan.con.peer, msg)
        else:
            self._toxnetWriteOffline(chan, msg)
        return

    def _toxnetWriteOffline(self, chan, msg):
        qDebug('here')
        chan.offline_buffers.append(msg)
        
        return

    def _toxnetResendOfflineMessages(self, chan):
        qDebug('here')

        cnter = 0
        while cnter < 10000 and len(chan.offline_buffers) > 0:
            msg = chan.offline_buffers.pop()
            self.toxkit.sendMessage(chan.con.peer, msg)
            cnter += 1
            pass
        qDebug('resend bufcnt: %d' % cnter)
        
        return

    def _onTcpConnected(self):
        qDebug('here')

        sock = self.sender()
        chan = self.chans[sock]

        reply = {
            'cmd': 'connect',
            'res': True,
            'chano': chan.chano,
            'cmdno': chan.cmdno,
        }

        reply = json.JSONEncoder().encode(reply)

        haspending = len(chan.offline_buffers) > 0
        status = self.toxkit.friendGetConnectionStatus(chan.con.peer)
        if status > 0 and haspending is False:
            self.toxkit.sendMessage(chan.con.peer, reply)
        else:
            self._toxnetWriteOffline(chan, reply)
        
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
        msg = {
            'cmd': 'close',
            'chano': chan.chano,
            'cmdno': cmdno,
        }
        
        msg = json.JSONEncoder().encode(msg)

        haspending = len(chan.offline_buffers) > 0
        status = self.toxkit.friendGetConnectionStatus(chan.con.peer)
        if status > 0 and haspending is False:
            self.toxkit.sendMessage(chan.con.peer, msg)
        else:
            self._toxnetWriteOffline(chan, msg)
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

        cnter = 0
        tlen = 0
        while sock.bytesAvailable() > 0:
            bcc = sock.read(128)
            self._toxnetWrite(chan, bcc)
            chan.rdlen += len(bcc)
            cnter += 1
            tlen += len(bcc)
            
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

import sys, time

from PyQt5.QtCore import *
from PyQt5.QtNetwork import *
from PyQt5.QtWidgets import *

import qtutil
from ui_mainwindow import *

from pytox import *

class MainWindow(QMainWindow):

    def __init__(self, parent = None):
        super(MainWindow, self).__init__()
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)

        
        return


SERVER = [
    "54.199.139.199",
    33445,
    "7F9C31FE850E97CEFD4C4591DF93FC757C7C12549DDD55F8EEAECC34FE76C029"
]

class ToxOptions():
    def __init__(self):
        self.ipv6_enabled = True
        self.udp_enabled = True
        self.proxy_type = 0 # 1=http, 2=socks
        self.proxy_host = ''
        self.proxy_port = 0
        self.start_port = 0
        self.end_port = 0
        self.tcp_port = 0
        self.savedata_type = 0 # 1=toxsave, 2=secretkey
        self.savedata_data = b''
        self.savedata_length = 0
        
class ToxDhtServer():
    def __init__(self):
        self.addr = ''
        self.port = -1
        self.pubkey = ''
        self.name = ''
        return

class ToxSettings():
    def __init__(self):
        self.sdir = '/home/gzleo/.config/tox';
        self.path = self.sdir + '/qtox.ini'
        self.qsets = QSettings(self.path, QSettings.IniFormat)
        self.data = self.sdir + '/tkdata'
        
        return

    def getDhtServerList(self):
        self.qsets.beginGroup('DHT Server/dhtServerList')
        stsize = int(self.qsets.value('size'))

        dhtsrvs = []
        for i in range(1, stsize+1):
            dhtsrv = ToxDhtServer()
            dhtsrv.addr = self.qsets.value('%d/address' % i)
            dhtsrv.port = int(self.qsets.value('%d/port' % i))
            dhtsrv.pubkey = self.qsets.value('%d/userId' % i)
            dhtsrv.name = self.qsets.value('%d/name' % i)
            dhtsrvs.append(dhtsrv)
            
        return dhtsrvs

    def getSaveData(self):
        print(self.data)
        fp = QFile(self.data)
        fp.open(QIODevice.ReadOnly)
        data = fp.readAll()
        fp.close()
        return data.data()

    def saveData(self, data):
        fp = QFile(self.data)
        fp.open(QIODevice.ReadWrite | QIODevice.Truncate)
        n = fp.write(data)
        fp.close()
        
        return n


class ToxSlots(Tox):
    def __init__(self, opts):
        super(ToxSlots, self).__init__(opts)
        self.opts = opts

        #self.fwd_friend_request = None
        #self.fwd_connection_status = None
        
        return

    # def on_friend_request(self, pubkey, data):
    #     qDebug(str(pubkey))
    #     qDebug(str(data))
    #     if self.fwd_friend_request != None:
    #         self.fwd_friend_request(pubkey, data)
    #     return

    # def on_connection_status(self):
    #     qDebug('hereh')
    #     if self.fwd_connection_status != None:
    #         self.fwd_connection_status()
    #     return

### 支持qt signal slots
class QToxKit(QThread):
    friendRequest = pyqtSignal('QString', 'QString')
    connectChanged = pyqtSignal(bool)
    newMessage = pyqtSignal('QString', 'QString')
    
    def __init__(self, parent = None):
        super(QToxKit, self).__init__(parent)
        self.sets = ToxSettings()

        self.opts = ToxOptions()
        self.stopped = False
        self.connected = False
        self.bootstrapStartTime = None
        self.bootstrapFinishTime = None
        self.first_connected = True
        
        self.tox = Tox(self.opts)
        self.tox = None

        self.start()
        return

    def run(self):
        self.makeTox()
        
        self.bootstrapStartTime = QDateTime.currentDateTime()
        self.bootDht()

        # self.exec_()
        while self.stopped != True:
            self.itimeout()
            QThread.msleep(self.tox.do_interval() * 9)

        qDebug('toxkit thread exit.')
        return

    def makeTox(self):
        self.opts.savedata_data = self.sets.getSaveData()
        print(type(self.opts.savedata_data))
        print(len(self.opts.savedata_data), self.opts.savedata_data[0:32])
        
        self.tox = ToxSlots(self.opts)
        myaddr = self.tox.get_address()
        self.tox.set_name('tki.' + myaddr[0:5])
        print(str(self.tox.get_address()))
        newdata = self.tox.save()
        print(len(newdata), newdata[0:32])
        self.sets.saveData(newdata)

        # callbacks
        self.tox.on_friend_request = self.fwdFriendRequest
        self.tox.on_connection_status = self.onConnectStatus
        self.tox.on_friend_message = self.onFriendMessage
        self.tox.on_user_status = self.onFriendStatus
        
        return
    
    def bootDht(self):
        dhtsrvs = self.sets.getDhtServerList()
        sz = len(dhtsrvs)
        qsrand(time.time())
        rndsrvs = {}
        while True:
            rnd = qrand() % sz
            rndsrvs[rnd] = 1
            if len(rndsrvs) >= 5: break

        qDebug('selected srvs:' + str(rndsrvs))
        for rnd in rndsrvs:
            srv = dhtsrvs[rnd]
            #qDebug('bootstrap from:' + str(rndsrvs) +  str(srv))
            qDebug('bootstrap from: %s %d %s' % (srv.addr, srv.port, srv.pubkey))
            bsret = self.tox.bootstrap_from_address(srv.addr, srv.port, srv.pubkey)
            rlyret = self.tox.add_tcp_relay(srv.addr, srv.port, srv.pubkey)

        return
    
    def itimeout(self):
        civ = self.tox.do_interval()

        self.tox.do()
        conned = self.tox.isconnected()
        #qDebug('hehre' + str(conned))
        
        if conned != self.connected:
            qDebug('connect status changed: %d -> %d' % (self.connected, conned))
            if conned is True: self.bootstrapFinishTime = QDateTime.currentDateTime()
            self.connected = conned
            self.connectChanged.emit(conned)
            self.onSelfConnectStatus(conned)
           
        return

    def fwdFriendRequest(self, pubkey, data):
        qDebug(str(pubkey))
        qDebug(str(data))

        fnum = self.tox.add_friend_norequest(pubkey)
        qDebug(str(fnum))
        
        # self.tox.send_message(fnum, 'hehe accept')
        
        return
    
    def onConnectStatus(self, fno, status):
        qDebug('hehre: fnum=%s, status=%s' % (str(fno), str(status)))
        
        
        return

    def onSelfConnectStatus(self, status):
        qDebug('my status: %s' % str(status))
        fnum = self.tox.count_friendlist()
        qDebug('friend count: %d' % fnum)
        # 为什么friend count是0个呢？，难道是因为没有记录吗？
        # 果然是这个样子的
        
        friends = ['398C8161D038FD328A573FFAA0F5FAAF7FFDE5E8B4350E7D15E6AFD0B993FC529FA90C343627',
                   '4610913CF8D2BC6A37C93A680E468060E10335178CA0D23A33B9EABFCDF81A46DF5DDE32954A',
        ]

        if status is True and self.first_connected:
            self.first_connected = False
            for friend in friends:
                self.tox.add_friend_norequest(friend)
        
        return
    
    def onFriendMessage(self, fno, msg):
        u8msg = msg.encode('utf8') # str ==> bytes
        #print(u8msg)
        u8msg = str(u8msg, encoding='utf8')
        #print(u8msg) # ok, python utf8 string
        qDebug(u8msg.encode('utf-8')) # should ok, python utf8 bytes
        
        fid = self.tox.get_client_id(fno)
        print('hehre: fnum=%s, fid=%s, msg=' % (str(fno), str(fid)), u8msg)
        self.newMessage.emit(fid, msg)
        return

    def onFriendStatus(self, fno, status):
        qDebug('hehre: fnum=%s, status=%s' % (str(fno), str(status)))
        return

class ShellBox(QObject):
    def __init__(self, parent = None):
        super(ShellBox, self).__init__(parent)
        self.toxkit = QToxKit()
        return
    
def main():
    a = QApplication(sys.argv)
    qtutil.pyctrl()

    qDebug('aaa'.encode('utf8') + '成'.encode('utf-8'))

    mw = MainWindow()
    # mw.show()

    shbox = ShellBox()

    qDebug('execing...')
    return a.exec_()

if __name__ == '__main__':
    main()
    

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
        self.data = self.sdir + '/data'
        
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
        fp = QFile(self.data)
        fp.open(QIODevice.ReadOnly)
        data = fp.readAll()
        fp.close()
        return data.data()

### 支持qt signal slots
class QToxKit(QThread):
    connectChanged = pyqtSignal(bool)
    
    def __init__(self, parent = None):
        super(QToxKit, self).__init__(parent)
        self.sets = ToxSettings()
        self.stopped = False
        self.connected = False
        self.bootstrapStartTime = None
        self.bootstrapFinishTime = None

        self.tox = Tox()

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
        self.tox = Tox('123', 6)
        
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
           
        return

class ShellBox(QObject):
    def __init__(self, parent = None):
        super(ShellBox, self).__init__(parent)
        self.toxkit = QToxKit()
        return
    
def main():
    a = QApplication(sys.argv)
    qtutil.pyctrl()

    mw = MainWindow()
    # mw.show()

    shbox = ShellBox()

    qDebug('execing...')
    return a.exec_()

if __name__ == '__main__':
    main()
    

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

class ShellBox(Tox):
    def __init__(self):
        # super(ShellBox, self).__init__()
        self.set_name('ShellBox')
        print('ID: %s' % self.get_address())
        
        self.connect()
        return

    #dhtServerList\1\userId=951C88B7E75C867418ACDB5D273821372BB5BD652740BCDF623A4FA293E75D2F
    #dhtServerList\1\address=192.254.75.98
    #dhtServerList\1\port=33445
    #dhtServerList\9\name=nurupo
    #dhtServerList\9\userId=F404ABAA1C99A9D37D61AB54898F56793E1DEF8BD46B1038B9D822E8460FAB67
    #dhtServerList\9\address=192.210.149.121
    #dhtServerList\9\port=33445

    def connect(self):
        a = self.bootstrap_from_address("192.210.149.121", 33445,
                                        "F404ABAA1C99A9D37D61AB54898F56793E1DEF8BD46B1038B9D822E8460FAB67")
        print(a)
        qDebug(str(self.isconnected()))
        return
    
def main():
    a = QApplication(sys.argv)
    qtutil.pyctrl()

    mw = MainWindow()
    # mw.show()

    shbox = ShellBox()

    while True:
        qDebug(str(shbox.do_interval()))
        shbox.do()
        qDebug(str(shbox.isconnected()))
        QThread.msleep(shbox.do_interval() * 8)
        # time.sleep(1)
    
    qDebug('execing...')
    return a.exec_()

if __name__ == '__main__':
    main()
    

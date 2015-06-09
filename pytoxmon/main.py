import sys, time

from PyQt5.QtCore import *
from PyQt5.QtNetwork import *
from PyQt5.QtWidgets import *

from qtutil import *
from ui_dhtmon import *

class DhtMon(QMainWindow):

    def __init__(self, parent = None):
        super(DhtMon, self).__init__()
        self.ui = Ui_DhtMon()
        self.ui.setupUi(self)

        return

def main():
    a = QApplication(sys.argv)
    pyctrl()

    dm = DhtMon()
    dm.show()

    return a.exec_()

if __name__ == '__main__':
    main()
    

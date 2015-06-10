import sys, time

from PyQt5.QtCore import *
from PyQt5.QtNetwork import *
from PyQt5.QtWidgets import *

import qtutil
from ui_mainwindow import *
from qtoxkit import *


class MainWindow(QMainWindow):

    def __init__(self, parent = None):
        super(MainWindow, self).__init__()
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)

        
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
    

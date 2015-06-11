import sys, time

from PyQt5.QtCore import *
from PyQt5.QtNetwork import *
from PyQt5.QtWidgets import *

from qtoxkit import *
import qtutil


class CmdInfo():
    def __init__(self, peer, cmd):
        self.peer = peer
        self.cmd = cmd
        return
    

class CmdAct(QObject):

    def __init__(self, parent = None):
        super(CmdAct, self).__init__(parent)
        self.builtin_cmds = ['/help', '/kill']
        self.cmds = []
        self.proc = QProcess()
        self.proc.finished.connect(self.onFinished)
        self.proc.readyReadStandardError.connect(self.onReadyRead)
        self.proc.readyReadStandardOutput.connect(self.onReadyRead)
        
        self.toxkit = QToxKit()
        self.toxkit.newMessage.connect(self.onNewMessage)
        self.peerid = ''
        
        return

    def onNewMessage(self, peer, msg):
        qDebug(peer)
        qDebug(msg)

        if msg in self.builtin_cmds: return self.actBuiltins(peer, msg)
        
        self.peerid = peer

        self.toxkit.sendMessage(peer, '+ ' + msg)
        self.cmds.append(msg)
        self.actNext()
        
        return

    def actNext(self):
        state = self.proc.state()
        if state == QProcess.NotRunning:
            if len(self.cmds) > 0:
                cmd = self.cmds.pop()
                #cmdpts = cmd.split(' ')
                self.proc.start(cmd)
        return
    
    def onReadyRead(self):
        stdout_data = self.proc.readAllStandardOutput()
        stderr_data = self.proc.readAllStandardError()
        if len(stdout_data) > 0:
            self.toxkit.sendMessage(self.peerid, stdout_data)
        if len(stderr_data) > 0:
            self.toxkit.sendMessage(self.peerid, stderr_data)
        return

    def onFinished(self, exitCode, exitStatus):
        self.actNext()
        return

    def actBuiltins(self, peer, cmd):
        if cmd == '/help':
            self.toxkit.sendMessage(peer, 'help not ready.')
        elif cmd == '/kill':
            self.proc.terminate()
        else:
            qDebug('not impled: ' + cmd)
        return
    

def main():
    a = QApplication(sys.argv)
    qtutil.pyctrl()

    act = CmdAct()

    qDebug('execing...')
    return a.exec_()

if __name__ == '__main__':
    main()
    

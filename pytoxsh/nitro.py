import sys, time, os

from PyQt5.QtCore import *
from PyQt5.QtNetwork import *
from PyQt5.QtWidgets import *

from qtoxkit import *
import qtutil


class RecordInfo():
    def __init__(self, peer, cmd):
        self.peer = peer
        self.cmd = cmd
        return

import subprocess
import pygit2

class NitroServer(QObject):

    def __init__(self, parent = None):
        super(NitroServer, self).__init__(parent)
        self.ddir = './nidat'
        self.builtin_cmds = ['/help', '/kill']
        self.cmds = []

        qDebug('libgit2 version: %s' % pygit2.LIBGIT2_VERSION) 
        
        self.toxkit = QToxKit()
        self.toxkit.newMessage.connect(self.onNewMessage)
        self.peerid = ''

        self.repo = None
        if os.path.exists(self.ddir) is False:
            self.initRepo()
            pass

        self.add('abcd中.txt', 'iiiii噗iiii')
        self.update('abcd中.txt', 'iii为进iiiiiiccccccccccccccccc')
        #self.delete('abcd中.txt')
        self.getList()
        self.getFile('abcd中.txt')
        
        return

    def initRepo(self):
        origin_url = 'git@git.oschina.net:kitech/toxnitro.git'
        cmdline = 'git clone %s %s' % (origin_url, self.ddir)
        subprocess.call(cmdline.split(' '))
        
        cmdline = 'git config user.email hehe@hehe.com'
        subprocess.call(cmdline.split(' '), cwd=self.ddir)
        cmdline = 'git config user.name toxbot'
        subprocess.call(cmdline.split(' '), cwd=self.ddir)
        
        #repo = pygit2.init_repository(self.ddir, origin_url = origin_url)
        #repo = pygit2.clone_repository(origin_url, self.ddir,
        #                               credentials = self.cbauth,
        #                               certificate = self.cbcert)
        #self.repo = repo
        
        return
    def cbauth(self, *args):
        qDebug(str(args))
        return True
    
    def cbcert(self, cert, valid, host):
        qDebug(str(cert))
        qDebug(str(valid))
        qDebug(str(host))
        return True

    def add(self, name, data):
        udata = data
        print(123, data, type(data), 456)

        udata = data.encode('utf8')
        qDebug(udata)
        #sys.exit()
        
        p = '%s/%s' % (self.ddir, name)
        fp = QFile(p)
        fp.open(QIODevice.ReadWrite)
        fp.write(QByteArray(udata))
        fp.resize(len(data))
        fp.close()

        cmdline = 'git add -v %s' % name
        subprocess.call(cmdline.split(' '), cwd=self.ddir)
        cmdline = 'git status'
        subprocess.call(cmdline.split(' '), cwd=self.ddir)
        cmdline = 'git commit -a -m toxbotadd'
        subprocess.call(cmdline.split(' '), cwd=self.ddir)
        return
    
    def delete(self, name):
        p = '%s/%s' % (self.ddir, name)
        QFile().remove(p)

        cmdline = 'git commit -a -m toxbotdel'
        subprocess.call(cmdline.split(' '), cwd=self.ddir)
        return

    def update(self, name, data):
        self.add(name, data)
        return

    def getList(self, name = None):
        if name is None: name = '.'
        p = '%s/%s' % (self.ddir, name)
        qDebug(p)
        d = QDir(p)
        elems = d.entryList()
        #qDebug(str(elems))
        print(elems)
        
        return

    def getFile(self, name):
        p = '%s/%s' % (self.ddir, name)
        fp = QFile(p)
        fp.open(QIODevice.ReadOnly)
        bcc = fp.readAll()
        fp.close()

        qDebug(str(bcc))

        return
    
    def onNewMessage(self, peer, msg):
        qDebug(peer)
        #qDebug(msg)

        self.peerid = peer
        self.toxkit.sendMessage(peer, '+ ' + msg)

        ### format: <add|del|update|list|get> <filename> <content>
        mps = msg.split(' ')
        cmd = mps[0]
        name = mps[1]
        bcc = msg[len(cmd)+len(name)+2:]

        if cmd == 'add':
            self.add(name, bcc)
        elif cmd == 'del':
            self.delete(name)
        elif cmd == 'update':
            self.update(name, bcc)
        elif cmd == 'list':
            self.getList(name)
        elif cmd == 'get':
            self.getFile(name)
        else:
            qDebug('unkown cmd %s' % cmd)

        self.toxkit.sendMessage(peer, '%s %s Done' % (cmd, name))
        
        #self.cmds.append(msg)
        #self.actNext()
        
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

    act = NitroServer()

    qDebug('execing...')
    return a.exec_()

if __name__ == '__main__':
    main()
    

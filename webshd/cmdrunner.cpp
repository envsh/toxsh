#include <pty.h>
#include <unistd.h>
#include <sys/select.h>

#include "cmdrunner.h"


CmdRunner::CmdRunner()
    : QThread(0)
{
    QObject::connect(this, &CmdRunner::started, this, &CmdRunner::onThreadStarted);
}

CmdRunner::~CmdRunner()
{

}

bool CmdRunner::init()
{
    int rc;

    m_win.ws_col = 80;
    m_win.ws_row = 24;
    m_win.ws_xpixel = 480;
    m_win.ws_ypixel = 192;

    rc = openpty(&m_fdm, &m_fds, m_ttyname, NULL, &m_win);
    qDebug()<<"openpty rc:"<<rc<<", pty:"<<m_ttyname;

    m_chpid = fork();
    if (m_chpid < 0) {
        qDebug()<<"fork error:";
    } else if (m_chpid == 0) {
        // child process;
        close(m_fdm);
        close(0); close(1); close(2);
        dup(m_fds); dup(m_fds); dup(m_fds);
        int brc = execvp("/usr/bin/bash", NULL);
        brc = ::write(m_fds, "hehe,done\n", 10);
        exit(brc);
    }

    // parent process
    close(m_fds);
    rc = ::write(m_fdm, "tty; sleep 1; \n", 15);

    return true;
}

void CmdRunner::run()
{
    int rc;
    char buf[256];
    int nfds = m_fdm + 1;
    fd_set rfds;
    
    while (true) {
        FD_ZERO(&rfds);
        FD_SET(m_fdm, &rfds);
        
        qDebug()<<"selecting...";
        rc = ::select(nfds, &rfds, NULL, NULL, NULL);
        qDebug()<<"selected:"<<rc;
        if (rc > 0) {
            memset(buf, 0, sizeof(buf));
            rc = ::read(m_fdm, buf, sizeof(buf));
            qDebug()<<"read rc:"<<rc<<", output:"<< (char*)buf;
        } else if (rc == 0) {

        } else {
            qDebug()<<"select pty master error.";
        }
    }
    
    this->exec();
}

void CmdRunner::onNewCommand(QString cmd)
{
    int rc = ::write(m_fdm, cmd.toLatin1().data(), cmd.length());
    qDebug()<<rc;
}

void CmdRunner::onThreadStarted()
{
    qDebug()<<"started";
    QString cmd = "ls; vim \ni";
    onNewCommand(cmd);
}


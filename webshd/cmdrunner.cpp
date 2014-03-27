#include <pty.h>
#include <unistd.h>
#include <sys/select.h>
#include <assert.h>

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
    // rc = ::write(m_fdm, "tty; sleep 1; \n", 15);

    return true;
}

void CmdRunner::run()
{
    QJsonObject jobj;
    int rc;

    while (true) {
        if (this->m_q_cmds.count() == 0) {
            sleep(1);
            continue;
        }

        jobj = this->m_q_cmds.dequeue();
        rc = ::write(m_fdm, jobj.value("cmd").toString().toLocal8Bit().data(), jobj.value("cmd").toString().length());
        assert(rc > 0);

        this->runOne(QString("%1").arg(jobj.value("id").toInt()));
    }

    assert(1 == 2);
    this->exec();
}

void CmdRunner::runOne(QString cmdid)
{
    int rc;
    char buf[256];
    int nfds = m_fdm + 1;
    fd_set rfds;
    struct timeval tv;

    QStringList outlist;
    QString  outstr;


    bool is_finished = false;
    while (true) {
        FD_ZERO(&rfds);
        FD_SET(m_fdm, &rfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        // qDebug()<<"selecting...";
        rc = ::select(nfds, &rfds, NULL, NULL, &tv);
        // qDebug()<<"selected:"<<rc;
        if (rc > 0) {
            memset(buf, 0, sizeof(buf));
            rc = ::read(m_fdm, buf, sizeof(buf));
            qDebug()<<"read rc:"<<rc<<", output:"<< (char*)buf;
            outstr.append(buf);


            outlist = outstr.split("\n");
            for (int i =0; i < outlist.count(); i ++) {
                if (outlist.at(i).startsWith("ENDOFCMD")) {
                    qDebug()<<"maybe cmd finished";
                    is_finished = true;
                }
            }
            if (outlist.count() >= 10 || this->m_last_send_time.secsTo(QDateTime::currentDateTime()) > 3) {
                emit stdoutReady(outstr, cmdid);
                outstr.clear();
            }

            if (is_finished) break;
        } else if (rc == 0) {
            // timeout
        } else {
            qDebug()<<"select pty master error.";
        }

    }

    if (outstr.length() > 0) {
        emit stdoutReady(outstr, cmdid);
        outstr.clear();
    }
}

void CmdRunner::onNewCommand(QJsonObject jcmd)
{
    //$this->_end_cmd = 'ENDOFCMD'.uniqid();
    // $this->_cmd_suffix = ' ; echo ' . $this->_end_cmd . ',,,$PWD,,,$OLDPWD' . "\n";

    QString end_cmd = QString("ENDOFCMD%1").arg(time(NULL));
    QString cmd_suffix = " ; echo " + end_cmd + ",,,$PWD,,,$OLDPWD\n";
    QString cmd = jcmd.value("cmd").toString() + cmd_suffix;

    jcmd.insert("cmd", QJsonValue(cmd));
    this->m_q_cmds.enqueue(jcmd);

    // int rc = ::write(m_fdm, cmd.toLatin1().data(), cmd.length());
    // qDebug()<<rc;
}

void CmdRunner::onThreadStarted()
{
    qDebug()<<"started";
    // QString cmd = "ls; vim \ni";
    // QString cmd = "ls;\n";
    // onNewCommand(cmd);

    QJsonDocument jdoc = QJsonDocument::fromJson("{\"id\":1,\"cmd\":\"ls\"}");
    QJsonObject jobj = jdoc.object();
    onNewCommand(jobj);
}


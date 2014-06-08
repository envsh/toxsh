
#include "commandhandler.h"

CommandHandler::CommandHandler()
    : QObject(0)
{
    m_proc = new QProcess();
    QObject::connect(m_proc, SIGNAL(error(QProcess::ProcessError)),
                     this, SLOT(onProcError(QProcess::ProcessError)));
    QObject::connect(m_proc, SIGNAL(finished(int, QProcess::ExitStatus)),
                     this, SLOT(onProcFinished(int, QProcess::ExitStatus)));
    QObject::connect(m_proc, &QProcess::readyReadStandardOutput, this, &CommandHandler::onStdoutReadyRead);
    QObject::connect(m_proc, &QProcess::readyReadStandardError, this, &CommandHandler::onStderrReadyRead);

    m_cmd_done_notifier = new QLocalServer();
    QObject::connect(m_cmd_done_notifier, &QLocalServer::newConnection,
                     this, &CommandHandler::onNewNotifier);
    QFile::remove("/tmp/toxshd.sock");
    bool bret = m_cmd_done_notifier->listen("/tmp/toxshd.sock");
    qDebug()<<"create unix socket notifier:"<<bret;
    

    QString prog = "bash";
    QStringList args = {};
    m_proc->start(prog, args);
}

CommandHandler::~CommandHandler()
{
}

bool CommandHandler::current_proc_want_input()
{
    QByteArray line = m_last_line;
    qDebug()<<"want input detect line:"<<line;

    // for pacman
    if (line.contains(" [Y/n]")) {
        return true;
    }

    QByteArray eline = m_last_error;
    qDebug()<<"want input detect eline:"<<eline;
    if (eline.contains(" [Y/n]")) {
        return true;
    }

    return false;
}

void CommandHandler::onNewCommand(QString cmd, int did)
{
    QString prog = "ls";
    QStringList args = {};

    QStringList tl = cmd.split(" ");    
    prog = tl.at(0);
    prog = "sh";
    for (int i = 0; i < tl.count(); i ++) {
        args << tl.at(i);
    }

    qDebug()<<"running cmd:"<<cmd<<did;
    // m_proc->start(prog, args);

    QString rfmt_cmd = cmd + QString(";echo ENDOFCMD_%1 | ncat -U  /tmp/toxshd.sock").arg(did);
    if (m_did == -1) {
        m_did = did;
    } else {
        if (current_proc_want_input()) {
            qDebug()<<"want input detected.";
            rfmt_cmd = cmd;
        } else {
            m_dids.enqueue(did);
        }
    }

    qint64 rc = m_proc->write(rfmt_cmd.toLatin1() + "\n");
    qDebug()<<"write cmdlen:"<<rc;
}

void CommandHandler::onProcError(QProcess::ProcessError error)
{
    QString msg;

    switch (error) {
    case QProcess::FailedToStart:
        msg = "failed to start";
        break;
    case QProcess::Crashed:
        msg = "crashed";
        break;
    case QProcess::UnknownError:
    default:
        msg = "unknown error";
        break;
    }

    emit this->commandResponeLine(m_did, msg);
}

void CommandHandler::onProcFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug()<<""<<exitCode;
    
    emit this->commandResponeFinished(m_did);
    m_did = -1;
}

void CommandHandler::onStdoutReadyRead()
{
    QByteArray ba = m_proc->readAllStandardOutput();
    int did = m_did;

    m_last_line += ba;
    if (m_last_line.length() > 128) {
        m_last_line = m_last_line.right(128);
    }

    emit this->commandResponeLine(did, QString(ba));

    // return;
    qint64 eoc = ba.indexOf("ENDOFCMD_");
    if (eoc != -1) {
        qDebug()<<"got end of cmd: "<<ba.mid(eoc + 9, 1); 
        if (!m_dids.isEmpty()) {
            m_did = m_dids.dequeue();
        } else {
            m_did = -1;
        }
        ba = ba.left(eoc);
    } else {
    }

    if (ba.length() > 0) {

    } else {
        qDebug()<<"empty line, maybe end of cmd."<<did<<ba;
    }
}

void CommandHandler::onStderrReadyRead()
{
    QByteArray ba = m_proc->readAllStandardError();
    qDebug()<<ba;

    m_last_error += ba;
    if (m_last_error.length() > 128) {
        m_last_error = m_last_error.right(128);
    }

    emit this->commandResponeLine(m_did, QString(ba));
}


void CommandHandler::onNewNotifier()
{
    qDebug()<<""<<sender();
    QLocalSocket *sock = m_cmd_done_notifier->nextPendingConnection();
    QObject::connect(sock, &QLocalSocket::readyRead, this, &CommandHandler::onNotifierReadyRead);
}

void CommandHandler::onNotifierReadyRead()
{
    QLocalSocket *sock = (QLocalSocket*)(sender());
    QByteArray ba = sock->readAll();

    qDebug()<<""<<sender()<<ba;

    if (!ba.startsWith("ENDOFCMD_")) {
        qDebug()<<"Unknown notify, droped";
        return;
    }

    qDebug()<<"got end of cmd: "<<ba; 
    if (!m_dids.isEmpty()) {
        m_did = m_dids.dequeue();
    } else {
        m_did = -1;
    }
    m_last_line.clear();
    m_last_error.clear();
}

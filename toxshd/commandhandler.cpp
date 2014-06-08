
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

    QString prog = "bash";
    QStringList args = {};
    m_proc->start(prog, args);
}

CommandHandler::~CommandHandler()
{
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

    if (m_did == -1) {
        m_did = did;
    } else {
        m_dids.enqueue(did);
    }
    QString rfmt_cmd = cmd + QString(";echo ENDOFCMD_%1").arg(did);
    m_proc->write(rfmt_cmd.toLatin1() + "\n");
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
        emit this->commandResponeLine(did, QString(ba));
    } else {
        qDebug()<<"empty line, maybe end of cmd."<<did<<ba;
    }
}

void CommandHandler::onStderrReadyRead()
{
    QByteArray ba = m_proc->readAllStandardError();
    qDebug()<<ba;

    emit this->commandResponeLine(m_did, QString(ba));
}





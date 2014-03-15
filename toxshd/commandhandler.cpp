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

    qDebug()<<"running cmd:"<<cmd;
    // m_proc->start(prog, args);

    m_proc->write(cmd.toLatin1() + "\n");
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

    emit this->commandResponeLine(0, msg);
}

void CommandHandler::onProcFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug()<<""<<exitCode;
    
    emit this->commandResponeFinished(0);
}

void CommandHandler::onStdoutReadyRead()
{
    QByteArray ba = m_proc->readAllStandardOutput();

    emit this->commandResponeLine(0, QString(ba));
}

void CommandHandler::onStderrReadyRead()
{
    QByteArray ba = m_proc->readAllStandardError();
    qDebug()<<ba;

    emit this->commandResponeLine(0, QString(ba));
}





#ifndef _COMMANDHANDLER_H_
#define _COMMANDHANDLER_H_

#include <QtCore>

class CommandHandler : public QObject
{
    Q_OBJECT;
public:
    CommandHandler();
    virtual ~CommandHandler();

public slots:
    void onNewCommand(QString cmd, int did);

signals:
    void commandResponeLine(int did, QString outline);
    void commandResponeFinished(int did);

private slots:
    void onProcError(QProcess::ProcessError error);
    void onProcFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onStdoutReadyRead();
    void onStderrReadyRead();

private:
    QProcess *m_proc = NULL;
};


#endif /* _COMMANDHANDLER_H_ */

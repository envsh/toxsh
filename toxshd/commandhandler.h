#ifndef _COMMANDHANDLER_H_
#define _COMMANDHANDLER_H_

#include <QtCore>
#include <QtNetwork>

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

    // 
    void onNewNotifier();
    void onNotifierReadyRead();
    
    // 
    bool current_proc_want_input();

private:
    QProcess *m_proc = NULL;
    QByteArray m_last_line;
    QByteArray m_last_error;
    int m_did = -1;
    QQueue<int> m_dids;
    QLocalServer  *m_cmd_done_notifier = NULL;
};


#endif /* _COMMANDHANDLER_H_ */

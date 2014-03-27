#ifndef _CMDRUNNER_H_
#define _CMDRUNNER_H_

#include <pty.h>

#include <QtCore>


class CmdRunner : public QThread
{
    Q_OBJECT;
public:
    CmdRunner();
    virtual ~CmdRunner();

    bool init();

    virtual void run();

public slots:
    void onNewCommand(QJsonObject jcmd);
    void onThreadStarted();

signals:
    void stdoutReady(QString out, QString cmdid);
    void stderrReady(QString out, QString cmdid);

private:
    void runOne(QString cmdid);

private:
    QProcess *m_sh = nullptr;
    int m_fdm = -1;
    int m_fds = -1;
    char m_ttyname[32] = {0};
    struct winsize m_win;
    struct termios m_tio;
    int m_chpid = -1;
    QDateTime m_last_send_time = QDateTime::currentDateTime();
    QQueue<QJsonObject> m_q_cmds;
};

#endif /* _CMDRUNNER_H_ */

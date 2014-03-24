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
    void onNewCommand(QString cmd);
    void onThreadStarted();

signals:
    void stdoutReady(QString out);
    void stderrReady(QString out);

private:
    QProcess *m_sh = nullptr;
    int m_fdm = -1;
    int m_fds = -1;
    char m_ttyname[32] = {0};
    struct winsize m_win;
    struct termios m_tio;
    int m_chpid = -1;
};

#endif /* _CMDRUNNER_H_ */

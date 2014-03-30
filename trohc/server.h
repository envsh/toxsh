#ifndef _SERVER_H_
#define _SERVER_H_

#include <QtCore>

class CmdProvider;
class CmdSender;
class VirtTcpC;

class Server : public QObject
{
    Q_OBJECT;
public:
    Server();
    virtual ~Server();

    bool init();

public slots:
    void onStdoutReady(QString out);
    void onStderrReady(QString out);

signals:
    void newCommand(QJsonObject jcmd);

private:
    CmdProvider *m_provider = NULL;
    CmdSender *m_sender = NULL;
    VirtTcpC *m_vtcpc = NULL;
};

#endif /* _SERVER_H_ */

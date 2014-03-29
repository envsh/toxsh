#ifndef _SERVER_H_
#define _SERVER_H_

#include <QtCore>

class VirtTcpD;
class CmdSender;

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
    VirtTcpD *m_vtcpd = NULL;
    CmdSender *m_sender = NULL;
};


#endif /* _SERVER_H_ */

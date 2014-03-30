#ifndef _CMDSENDER_H_
#define _CMDSENDER_H_

#include <QtCore>
#include <QtNetwork>

class CmdSender : public QObject
{
    Q_OBJECT;
public:
    CmdSender(int type);
    virtual ~CmdSender();
    bool init(int type);

    enum {CST_UNKNOWN = 0, CST_CPUSH, CST_SPUSH };

public slots:
    void onNewOutput(QString output, QString cmdid);
    void onRequestFinished(QNetworkReply *reply);
    void onPacketRecieved(QByteArray pkt);

private:
    void sendRequest(QByteArray data);

private:
    int m_type = CST_UNKNOWN;
    QNetworkAccessManager *m_nam = NULL;
    QQueue<QByteArray> m_resps;
    QMutex m_mutex;
    bool  m_requesting = false;
    QMap<QString, QString> m_hosts;
};

#endif /* _CMDSENDER_H_ */

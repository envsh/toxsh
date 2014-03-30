#ifndef _CMDRESPONSER_H_
#define _CMDRESPONSER_H_

#include <QtCore>
#include <QtNetwork>

class CmdResponser : public QObject
{
    Q_OBJECT;
public:
    CmdResponser();
    virtual ~CmdResponser();
    bool init();


public slots:
    void onNewOutput(QString output, QString cmdid);
    void onPacketRecieved(QByteArray pkt);
    void onRequestFinished(QNetworkReply *reply);

private:
    void sendRequest(QByteArray data);

private:
    QNetworkAccessManager *m_nam = NULL;
    QQueue<QByteArray> m_resps;
    QMutex m_mutex;
    bool  m_requesting = false;
    QMap<QString, QString> m_hosts;
};


#endif /* _CMDRESPONSER_H_ */

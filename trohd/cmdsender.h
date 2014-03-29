#ifndef _CMDSENDER_H_
#define _CMDSENDER_H_

#include <QtCore>
#include <QtNetwork>

class CmdSender : public QObject
{
    Q_OBJECT;
public:
    CmdSender();
    virtual ~CmdSender();
    bool init();
                           
public slots:
    void onNewOutput(QString output, QString cmdid);
    void onRequestFinished(QNetworkReply *reply);
    void onPacketRecieved(QByteArray ba);

private:
    QNetworkAccessManager *m_nam = NULL;
};

#endif /* _CMDSENDER_H_ */

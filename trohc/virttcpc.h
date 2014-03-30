#ifndef _VIRTTCPC_H_
#define _VIRTTCPC_H_

#include <QtCore>
#include <QtNetwork>

class VirtTcpC : public QObject
{
    Q_OBJECT;
public:
    VirtTcpC();
    virtual ~VirtTcpC();
    void init();
    void connectToHost();

public slots:
    void onClientReadyRead();
    void onClientDisconnected();
    void onClientError(QAbstractSocket::SocketError error);
    void onPacketRecievedStream(QByteArray pkt);
    void onPacketRecieved(QJsonObject jobj);
    
signals:
    void newPacket(QByteArray pkt);

private:
    QTcpSocket *m_sock = NULL;
};

#endif /* _VIRTTCPC_H_ */

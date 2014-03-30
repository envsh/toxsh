#ifndef _VIRTTCPD_H_
#define _VIRTTCPD_H_

#include <QtCore>
#include <QtNetwork>

class VirtTcpD : public QObject
{
    Q_OBJECT;
public:
    VirtTcpD();
    virtual ~VirtTcpD();
    void init();

public slots:
    void onNewConnection();
    void onClientReadyRead();
    // void onClientConnected();
    void onClientDisconnected();
    void onClientError(QAbstractSocket::SocketError socketError);
    // void onClientHostFound();
    // void onClientStateChanged(QAbstractSocket::SocketState socketState);
    void onPacketRecieved(QJsonObject jobj);

signals:
    void newPacket(QByteArray pkt);
    void realClientDisconnected();

private:
    QTcpServer *m_tcpd = NULL;
    QMap<QTcpSocket*, QTcpSocket*> m_conns;
};

#endif /* _VIRTTCPD_H_ */

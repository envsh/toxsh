#ifndef TOXTUNBASE_H
#define TOXTUNBASE_H

#include <QtCore>

class ToxTunChannel;

class ToxTunBase : public QObject
{
    Q_OBJECT;
    
public:
    uint32_t nextConid();

public:
    uint32_t m_conid = 7;
    QHash<QString, QVector<QByteArray> > m_pkts;  // friendId => [pkt1/2/3]

protected:
    virtual void promiseChannelCleanup(ToxTunChannel *chan) = 0;
                                               
public slots:
    void onToxnetFriendLossyPacket(QString friendId, QByteArray packet);
    void onToxnetFriendLosslessPacket(QString friendId, QByteArray packet);
public slots:
    void promiseCleanupTimeout();
};


#endif /* TOXTUNBASE_H */

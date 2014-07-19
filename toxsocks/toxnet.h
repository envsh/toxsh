#ifndef _TOXNET_H_
#define _TOXNET_H_

#include <QtCore>

#include <tox/tox.h>

#include "core.hpp"

// class CommandHandler;

class ToxNet : public QObject
{
    Q_OBJECT;
public:
    ToxNet();
    virtual ~ToxNet();

    void onConnected();
    void onDisconnected();
    void onFriendRequestReceived(const QString &userId, const QString &message);
    void onFriendAddressGenerated(QString friendAddress);
    void onFriendStatusChanged(int friendId, Status status);
    void onFailedToStartCore();

    //
    void messageReceived(int friendId, const QString& message);
    
public slots:
    // cmd resp
    void onCommandResponeLine(int did, QString oline);
    void onCommandResponeFinished(int did);

signals:
    void friendRequestAccepted(const QString& userId);
    void friendRequested(const QString& friendAddress, const QString& message);
    void packetReceived(QByteArray pkt, QString peer_addr);

private:
    Core* core = nullptr;
    QThread* coreThread = nullptr;
    // CommandHandler *m_hcmd = nullptr;
};

#endif /* _TOXNET_H_ */

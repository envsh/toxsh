#ifndef _SERVER_H_
#define _SERVER_H_

#include <QtCore>

#include <tox/tox.h>

#include "core.hpp"

class Server : public QObject
{
    Q_OBJECT;
public:
    Server();
    virtual ~Server();

    void onConnected();
    void onDisconnected();
    void onFriendRequestRecieved(const QString &userId, const QString &message);
    void onFailedToStartCore();

signals:
    void friendRequestAccepted(const QString& userId);
    void friendRequested(const QString& friendAddress, const QString& message);

private:
    Core* core = nullptr;
    QThread* coreThread = nullptr;
};

#endif /* _SERVER_H_ */

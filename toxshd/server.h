#ifndef _SERVER_H_
#define _SERVER_H_

#include <QtCore>

#include <tox/tox.h>

#include "core.hpp"

class CommandHandler;

class Server : public QObject
{
    Q_OBJECT;
public:
    Server();
    virtual ~Server();

    void onConnected();
    void onDisconnected();
    void onFriendRequestReceived(const QString &userId, const QString &message);
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

private:
    Core* core = nullptr;
    QThread* coreThread = nullptr;
    CommandHandler *m_hcmd = nullptr;
};

#endif /* _SERVER_H_ */

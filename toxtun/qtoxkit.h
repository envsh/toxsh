#ifndef QTOXKIT_H
#define QTOXKIT_H

#include <QtCore>

#include <tox/tox.h>

class ToxSettings;

class QToxKit : public QThread
{
    Q_OBJECT;
public:
    QToxKit(QString identifier = "anon", bool persist = true, QObject* parent = 0);
    virtual ~QToxKit();

    virtual void run();

    uint32_t friendAdd(QString friendId, QString requestMessage);
    uint32_t friendAddNorequest(QString friendId);
    bool friendDelete(QString friendId);
    void friendSendMessage(QString friendId, QByteArray data);
    bool friendSendLossyPacket(QString friendId, QByteArray data);
    bool friendSendLosslessPacket(QString friendId, QByteArray data);
    uint32_t friendByPublicKey(QString friendId);
    QString friendGetPublicKey(uint32_t friendNumber);

signals:
    void selfConnectionStatus(int status);
    void selfConnectChanged(bool);
    void selfConnected();
    void selfDisconnected();
    void friendRequest(QString, QString);
    void friendAdded(QString);
    void friendMessage(QString, int, QByteArray);
    void friendLossyPacket(QString friendId, QByteArray packet);
    void friendLosslessPacket(QString friendId, QByteArray packet);
    void friendConnected(QString);
    void friendConnectionStatus(QString, int);
    void friendStatus(QString, int);
    void fileRecv(QString, int, int, QString);
    void fileRecvControl(QString, int, int);
    void fileRecvChunk(QString, int, int, QString);
    void fileChunkRequest(QString, int, int, int);

    
public:
    

public:
    Tox *m_tox = NULL;
    ToxSettings *m_sets = NULL;
    bool m_first_connect = true;
};

#endif /* QTOXKIT_H */

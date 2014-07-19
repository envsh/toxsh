#ifndef _TUNNELD_H_
#define _TUNNELD_H_

#include <QtCore>
#include <QtNetwork>


class ToxNet;
class Srudp;

class Tunneld : public QObject
{
    Q_OBJECT;
public:
    Tunneld();
    virtual ~Tunneld();

    void init();

public slots:
    void onPeerConnected(int friendId);
    void onPeerDisconnected(int friendId);
    void onPeerReadyRead();

private slots:
    void onDestConnected();
    void onDestDisconnected();
    void onDestReadyRead();

private:
    ToxNet *m_net = NULL;
    Srudp *m_rudp = NULL;

    QTcpSocket *m_sock = NULL;
};

#endif /* _TUNNELD_H_ */

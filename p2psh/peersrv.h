#ifndef _PEERSRV_H_
#define _PEERSRV_H_


#include <QtCore>
#include <QtNetwork>

class StunClient;

class PeerSrv : public QObject
{
    Q_OBJECT;
public:
    PeerSrv();
    virtual ~PeerSrv();

    virtual void init();

public slots:
    virtual void onMappedAddressRecieved(QString addr);
    virtual void onRelayConnected();
    virtual void onRelayReadyRead();
    
protected:
    virtual QString getRegCmd() = 0;

protected:
    QString m_mapped_addr;
    StunClient *m_stun_client = NULL;
    QTcpSocket *m_rly_sock = NULL;
    
};

#endif /* _PEERSRV_H_ */

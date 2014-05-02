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

    void init();

public slots:
    void onMappedAddressRecieved(QString addr);
    void onRelayConnected();
    void onRelayReadyRead();
    
protected:
    QString m_mapped_addr;
    StunClient *m_stun_client = NULL;
    QTcpSocket *m_rly_sock = NULL;
    
};

#endif /* _PEERSRV_H_ */

#ifndef _XSHSRV_H_
#define _XSHSRV_H_


#include <QtCore>
#include <QtNetwork>

class StunClient;

class XshSrv : public QObject
{
    Q_OBJECT;
public:
    XshSrv();
    virtual ~XshSrv();

    void init();

public slots:
    void onMappedAddressRecieved(QString addr);
    void onRelayConnected();
    void onRelayReadyRead();
    void onBackendConnected();
    void onBackendReadyRead();

private:
    StunClient *m_stun_client = NULL;
    QTcpSocket *m_rly_sock = NULL;
    QTcpSocket *m_backend_sock = NULL;
    QString m_mapped_addr;
    
};

#endif /* _XSHSRV_H_ */

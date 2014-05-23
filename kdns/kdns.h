#ifndef _KDNS_H_
#define _KDNS_H_

#include <QtCore>
#include <QtNetwork>

class KDNS : public QObject
{
    Q_OBJECT;
public:
    explicit KDNS();
    virtual ~KDNS();

    void init();

public slots:
    void onReadyRead();
    void onFwdReadyRead();

private:
    QUdpSocket *m_sock = NULL;
    static const quint16 m_port = 53;
    QUdpSocket *m_fwd_sock = NULL;

    
};


#endif

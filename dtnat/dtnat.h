#ifndef _DTNAT_H_
#define _DTNAT_H_

#include <stdint.h>

#include <QtCore>
#include <QtNetwork>

typedef struct stun_header {
    uchar tz0 : 2;
    uint16_t msgtype : 14;
    uint16_t msglen : 16;
    uint32_t cookie : 32;
    uchar tranid[12]; // : 96
} stun_header;

typedef struct stun_attr {
    uint16_t len;
    uint16_t type;
    uchar value[1];
} stun_attr;

typedef struct stun_packet {
    stun_header hdr;
    stun_attr attr[1]; 
} stun_packet;

class DtNat : public QObject
{
    Q_OBJECT;
public:
    DtNat();
    ~DtNat();

    void test();
    void processResponse(QByteArray resp);

public slots:
    void onReadyRead();
    void onConnected();

private:
    QUdpSocket *msock = NULL;
};

/*
  非常全面的stun/turn实现，包括server和client。
  https://code.google.com/p/coturn/
  https://code.google.com/p/rfc5766-turn-server/
 */

#endif /* _DTNAT_H_ */

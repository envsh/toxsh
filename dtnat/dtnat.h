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

class DtNatBase : public QObject
{
    Q_OBJECT;
public:
    DtNatBase();
    virtual ~DtNatBase();

protected:
    void init_stuns();

protected:
    QVector<QString> m_stuns;
    QMap<int, QString> m_mapped_addrs; // stun_idx => mapped
};

class DtNatSrv : public DtNatBase
{
    Q_OBJECT;
public:
    DtNatSrv();
    virtual ~DtNatSrv();

    void init();
    void test();
    void processResponse(QByteArray resp);

    void do_phase1();
    void do_phase2();
    void do_phase3();
    static QString getStunAddress(QByteArray resp, uint16_t attr_type);

public slots:
    void onReadyRead();
    void onConnected();

    void onNotifyChannelConnected();
    void onNotifyChannelReadyRead();

    void onHoleTimeout();

private:
    QUdpSocket *m_detect_sock = NULL;
    QTcpSocket *m_notify_sock = NULL;

    QTimer *m_hole_timer = NULL;
    quint16 m_port_base = 0;
    QString m_hole_ip;
    QString m_hole_cmd;

    QUdpSocket *m_hole_sock = NULL;
    QString m_hole_addr_s1;
    QString m_hole_addr_s2;
    enum {PM_UNKNOWN = 0, PM_INC, PM_DEC, PM_SKIP};
    int m_punch_mode;
};

class DtNatCli : public DtNatBase
{
    Q_OBJECT;
public:
    DtNatCli();
    virtual ~DtNatCli();

    void init();

public slots:
    void onBrgConnected();
    void onBrgReadyRead();

    void onDetectReadyRead();
    void onHoleTimeout();
    void onHoleReadyRead();

private:
    void do_phase1();

private:
    QTcpSocket *m_brg_sock = NULL;
    QUdpSocket *m_detect_sock = NULL;
    QUdpSocket *m_hole_sock = NULL;
    QTimer *m_hole_timer = NULL;
    QString m_hole_addr_s1;
    QString m_hole_addr_s2;
    QMap<int, QUdpSocket*> m_hole_peers;
};

/*
  非常全面的stun/turn实现，包括server和client。
  https://code.google.com/p/coturn/
  https://code.google.com/p/rfc5766-turn-server/
  http://blog.csdn.net/oonukeoo/article/details/5942452
  http://blog.csdn.net/yu_xiang/article/details/9227023  STUN和TURN技术浅析  但内容算很详细了
  http://www.h3c.com.cn/MiniSite/Technology_Circle/Net_Reptile/The_Five/Home/Catalog/201206/747038_97665_0.htm

  http://www.bford.info/pub/net/p2pnat/

  how Skype gets through firewalls 
     http://makezine.com/2007/06/01/udp-hole-punching-how-skype-ge/
     http://www.h-online.com/security/features/How-Skype-Co-get-round-firewalls-747197.html 比较初级的，简单。


  https://github.com/ckennelly/hole-punch

  UDP multihole punching

  Maybe this link could be useful for someone:

  http://www.goto.info...ei-apan-v10.pdf

  it describes an idea of "multihole punching" which should be able to connect two peers who are both behind symmetric NAT, it needs two servers with public IP address to work though.

  I believe that this could be beneficial to developers and it fits into this thread. If this is already used, then please let us know.


 */

#endif /* _DTNAT_H_ */

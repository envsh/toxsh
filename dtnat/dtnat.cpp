
#include "ns_turn_defs.h"
#include "ns_turn_msg_defs.h"
#include "ns_turn_msg.h"
#include "ns_turn_msg_addr.h"
#include "ns_turn_ioaddr.h"
#include "ns_turn_utils.h"

#include "stun_buffer.h"

#include "dtnat.h"

#define NH_SERVER_ADDR "81.4.106.67"
#define NH_SERVER_PORT 7890

#define STUN_SERVER_ADDR1 "74.125.23.127" // "74.125.25.127" // "74.125.137.127" // "173.194.76.127" // "74.125.131.127" // "208.97.25.20"   // "132.177.123.6" // "66.228.45.110"
#define STUN_SERVER_PORT1 19302 // 3478

#define STUN_SERVER_ADDR2 "175.41.167.79"
#define STUN_SERVER_PORT2 3478



DtNat::DtNat(const char *name)
    : QObject(0)
{
    m_name = name;
    if (m_name == "server") {
        m_is_server = true;
    }
}

DtNat::~DtNat()
{
}

void DtNat::init()
{
    m_reg_sock = new QUdpSocket();
    QObject::connect(m_reg_sock, &QUdpSocket::connected, this, &DtNat::onRegChannelConnected);
    QObject::connect(m_reg_sock, &QUdpSocket::readyRead, this, &DtNat::onRegChannelReadyRead);
    QObject::connect(m_reg_sock, &QUdpSocket::bytesWritten, this, &DtNat::onRegChannelBytesWritten);
    m_reg_sock->bind(QHostAddress::AnyIPv4, 7766);

    int fd = m_reg_sock->socketDescriptor();
    int ttl = 10;
    setsockopt(fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));

    m_notify_sock = new QTcpSocket();
    QObject::connect(m_notify_sock, &QTcpSocket::connected, this, &DtNat::onNotifyChannelConnected);
    QObject::connect(m_notify_sock, &QTcpSocket::readyRead, this, &DtNat::onNotifyChannelReadyRead);

    // m_notify_sock->connectToHost(NH_SERVER_ADDR, NH_SERVER_PORT);

    for (int i = 1 ; i <= 3; i ++) {
        QString reg_str = QString("register%1;%2;hh;cc").arg(i).arg(m_name);
        QHostAddress serv_addr(NH_SERVER_ADDR);
        m_reg_sock->writeDatagram(reg_str.toLatin1().data(), reg_str.length(), serv_addr, NH_SERVER_PORT+i-1);
    }
}

void DtNat::test()
{
    msock = new QUdpSocket();
    QObject::connect(msock, &QUdpSocket::connected, this, &DtNat::onConnected);
    QObject::connect(msock, &QUdpSocket::readyRead, this, &DtNat::onReadyRead);
    msock->bind(QHostAddress::AnyIPv4, 7755);

    // QHostAddress addr("66.228.45.110");// ("numb.viagenie.ca"); // OK
    // QHostAddress addr("numb.viagenie.ca");
    // QHostAddress addr("192.168.1.103");
    // QHostAddress addr("208.97.25.20"); // stun.ideasip.com
    // QHostAddress addr("77.72.174.163"); // stun.voxgratia.org // XXX,低版本协议rfc3889
    // QHostAddress addr(""); // stun.voxalot.com        //xxx,低版本协议rfc3889
    // stun:stun.l.google.com:19302  // OKKKK    
    // stun:stun1.l.google.com:19302  // OKKKK    
    // stun:stun2.l.google.com:19302  // OKKKK    
    // stun:stun3.l.google.com:19302  // OKKKK    
    // stun:stun4.l.google.com:19302  // OKKKK
    // QHostAddress addr("77.72.174.161"); // stun.voipbuster.com // xxx,低版本协议rfc3889
    // QHostAddress addr("217.10.68.152"); // stun.sipgate.net //xxx,低版本协议rfc3889
    // QHostAddress addr("132.177.123.6"); // stunserver.org // xxx,低版本协议rfc3889
    // QHostAddress addr("77.72.174.167"); // stun.ekiga.net
    // QHostAddress addr("193.28.184.4"); // stun.ipshka.com
    // QHostAddress addr("198.27.81.168"); // stun.callwithus.com
    // QHostAddress addr("82.129.27.63"); // xxx,低版本协议rfc3889
    // QHostAddress addr("113.32.111.127"); // xxx,低版本协议rfc3889
    // QHostAddress addr("216.93.246.16"); // xxx
    QHostAddress addr("175.41.167.79"); //  stun.vline.com  // OKKKKK
    // QHostAddress addr("217.10.68.152"); // stun.faktortel.com.au // xxx,低版本协议rfc3889

    // 210.78.137.42:58701, 210.78.137.42:58765

    quint16 port = 3478;

    // msock->connectToHost("numb.viagenie.ca", port);

    stun_header header;
    memset(&header, 0, sizeof(header));
    header.tz0 = 0;
    header.msgtype = 0x0001;
    header.msglen = 5;
    // const static UInt32 StunMagicCookie  = 0x2112A442;  
    header.cookie = 0x2112A442;
    header.tranid[11] = 0x01;

    stun_packet pkt;
    memcpy(&pkt.hdr, &header, sizeof(header));

    u08bits message_type = stun_make_type(STUN_METHOD_BINDING);
    size_t rlen = 0;
    u08bits	rbuf2[STUN_BUFFER_SIZE];
    u08bits *rbuf = rbuf2;

    memset(rbuf, 0, STUN_BUFFER_SIZE);
    stun_init_request_str(STUN_METHOD_BINDING, rbuf, &rlen);
    // stun_set_binding_request_str(rbuf, &rlen);
    // stun_set_binding_request_str(buf->buf, (size_t*)(&(buf->len)));

    stun_buffer tbuf;
    // stun_prepare_binding_request(&tbuf);
    stun_set_binding_request_str(tbuf.buf, (size_t*)(&(tbuf.len)));

    qDebug()<<memcmp(rbuf, tbuf.buf, tbuf.len);

    memcpy(rbuf, tbuf.buf, tbuf.len);

    qDebug()<<memcmp(rbuf, tbuf.buf, tbuf.len);
    
    QByteArray msg = "01jjjjjjjjjjjjjjjjjjjjj2345985u6486456u4564586u4586u4586u496u486u8";
    quint64 wlen;// = msock->writeDatagram(msg, addr, port);
    quint64 wlen2 = msock->writeDatagram(QByteArray((char*)rbuf, tbuf.len), addr, port);
    // quint64 wlen2 = msock->writeDatagram(QByteArray((char*)&tbuf.buf, tbuf.len), addr, port);
    qDebug()<<addr<<port<<wlen<<wlen2<<rlen<<tbuf.len<<memcmp(rbuf, tbuf.buf, tbuf.len);
}

void DtNat::onConnected()
{
    QUdpSocket *sock = (QUdpSocket*)(sender());
    qDebug()<<""<<sock;

    QHostAddress addr("66.228.45.110");// ("numb.viagenie.ca");
    quint16 port = 3478;

    QByteArray msg = "01jjjjjjjjjjjjjjjjjjjjj2345985u6486456u4564586u4586u4586u496u486u8";
    quint64 wlen = msock->writeDatagram(msg, addr, port);
    qDebug()<<addr<<port<<wlen;
}

void DtNat::onReadyRead()
{
    QUdpSocket *sock = (QUdpSocket*)(sender());
    qDebug()<<""<<sock;

    u08bits rbuf[STUN_BUFFER_SIZE];

    while (sock->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(sock->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        sock->readDatagram(datagram.data(), datagram.size(),
                                &sender, &senderPort);

        qDebug()<<"read: "<<sender<<senderPort<<datagram.length()<<datagram.toHex();
        
        for (int i = 0; i < datagram.length(); i ++) {
            fprintf(stderr, "%c", datagram.at(i));
        }
        fprintf(stderr, "]\n");

        this->processResponse(datagram);
        // processTheDatagram(datagram);
    }
}

void DtNat::processResponse(QByteArray resp)
{
    u08bits rbuf[STUN_BUFFER_SIZE];
    size_t rlen = 0;
    stun_buffer buf;

    rlen = resp.length();
    memcpy(rbuf, resp.data(), resp.length());

    buf.len = resp.length();
    memcpy(buf.buf, resp.data(), resp.length());

	if (stun_is_command_message(&buf)) {
        if (stun_is_response(&buf)) {
            if (stun_is_success_response(&buf)) {
                if (stun_is_binding_response(&buf)) {
                    ioa_addr reflexive_addr;
                    addr_set_any(&reflexive_addr);
                    if (stun_attr_get_first_addr(&buf, STUN_ATTRIBUTE_XOR_MAPPED_ADDRESS, &reflexive_addr, NULL) >= 0) {
                        stun_attr_ref sar = stun_attr_get_first_by_type_str(buf.buf, buf.len, STUN_ATTRIBUTE_OTHER_ADDRESS);
                        if (sar) {
                            // *rfc5780 = 1;
                            printf("\n========================================\n");
                            // printf("RFC 5780 response %d\n",++counter);
                            ioa_addr other_addr;
                            stun_attr_get_addr_str((u08bits *) buf.buf, (size_t) buf.len, sar, &other_addr, NULL);
                            sar = stun_attr_get_first_by_type_str(buf.buf, buf.len, STUN_ATTRIBUTE_RESPONSE_ORIGIN);
                            if (sar) {
                                ioa_addr response_origin;
                                stun_attr_get_addr_str((u08bits *) buf.buf, (size_t) buf.len, sar, &response_origin, NULL);
                                addr_debug_print(1, &response_origin, "Response origin: ");
                            }
                            addr_debug_print(1, &other_addr, "Other addr: ");
                        }
                        addr_debug_print(1, &reflexive_addr, "UDP reflexive addr");
                    } else {
                        printf("Cannot read the response\n");
                    }
                } else {
                    printf("Wrong type of response\n");
                }
            } else {
                int err_code = 0;
                u08bits err_msg[1025] = "\0";
                size_t err_msg_size = sizeof(err_msg);
                if (stun_is_error_response(&buf, &err_code, err_msg, err_msg_size)) {
                    printf("The response is an error %d (%s)\n", err_code, (char*) err_msg);
                } else {
                    printf("The response is an unrecognized error\n");
                }
            }
        } else {
            printf("The response is not a reponse message\n");
        }
    } else {
        printf("The response is not a STUN message\n");
    }
}

void DtNat::onRegChannelConnected()
{
    qDebug()<<sender();
}

void DtNat::onRegChannelReadyRead()
{
    QUdpSocket *sock = (QUdpSocket *)(sender());
    QByteArray ba;
    char buff[256] = {0};
    QHostAddress peer_addr;
    quint16 peer_port;

    sock->readDatagram(buff, sizeof(buff), &peer_addr, &peer_port);
    qDebug()<<sender()<<buff<<peer_addr<<peer_port;

    QString cmd, from, to, value;
    QString ip;
    quint16 port;

    QList<QByteArray> elems = QByteArray(buff).split(';');
    cmd = elems.at(0);
    from = elems.at(1);
    to = elems.at(2);
    value = elems.at(3);

    if (cmd == "punch_do") {
        qDebug()<<"haha, hole punch okkkkk,"<<peer_addr<<peer_port;
    }
}

void DtNat::onRegChannelBytesWritten(qint64 bytes)
{
    static int runed = 0;
    if (runed) return;

    runed = 1;
    if (m_notify_sock->state() != QAbstractSocket::SocketState::ConnectedState) {
        qDebug()<<sender()<<bytes;
        m_notify_sock->connectToHost(NH_SERVER_ADDR, NH_SERVER_PORT);
    }
}

void DtNat::onNotifyChannelConnected()
{
    qDebug()<<sender();

    if (m_is_server) {
        
    } else {
        QString cmd_str = "punch_relay;client;server;dd";
        m_reg_sock->writeDatagram(cmd_str.toLatin1().data(), cmd_str.length(), QHostAddress(NH_SERVER_ADDR), NH_SERVER_PORT);

    }
}

void DtNat::onNotifyChannelReadyRead()
{
    QTcpSocket *sock = (QTcpSocket*)(sender());
    QByteArray ba = sock->readAll();
    qDebug()<<sender()<<ba<<"\n";
    int pnum = 25600;
    QString cmd, from, to, value;
    QString ip;
    quint16 port;

    QList<QByteArray> elems = ba.split(';');
    cmd = elems.at(0);
    from = elems.at(1);
    to = elems.at(2);
    value = elems.at(3);

    
    if (cmd == "punch_info") {
        QList<QString> elems2 = value.split(':');
        ip = elems2.at(0);
        qDebug()<<"peer addr:"<<elems2;
        port = elems2.at(1).toUShort();
        QString cmd_str = QString("punch_do;client;server;%1").arg(qrand());

        m_hole_cmd = cmd_str;
        m_port_base = port;
        m_hole_ip = ip;
        if (m_hole_timer == NULL) {
            m_hole_timer = new QTimer();
            QObject::connect(m_hole_timer, &QTimer::timeout, this, &DtNat::onHoleTimeout);
            m_hole_timer->setInterval(350);
            m_hole_timer->start();
        }
        
        /*
        qDebug()<<10<<"try holing .."<<ip<<port<<(port+pnum);
        for (int i = 0; i < 1000; i ++) {
            for (quint16 nport = port + 1; nport < port + 1 + pnum; nport ++) {
                m_reg_sock->writeDatagram(cmd_str.toLatin1().data(), cmd_str.length(), QHostAddress(ip), nport);
            }
            QThread::msleep(5);
        }
        qDebug()<<10<<"try holing .."<<ip<<port<<(port+pnum)<<"done";
        */
    }    

    if (m_is_server) {
        
    } else {

    }
}

void DtNat::onHoleTimeout()
{
    int pnum = 15600;
    quint16 port = m_port_base;
    QString cmd_str = m_hole_cmd;
    QString ip = m_hole_ip;
    quint64 iret;
    int cnter = 0;

    qDebug()<<10<<"try holing ..."<<port<<(port+pnum);    
    // for (quint16 nport = port; nport < port + 1 + pnum; nport ++) {
    // for (quint16 nport = 1025; nport < 65535; nport ++) {
    for (quint16 nport = 1025 + qrand() % (pnum - 1000); nport > 1024 && nport < 65535; nport += (qrand() % 8 + 1) ) {
        iret = m_reg_sock->writeDatagram(cmd_str.toLatin1(), QHostAddress(ip), nport);
        if (iret == -1) {
            qDebug()<<cnter<<"send udp error:"<<ip<<nport<<m_reg_sock->error()<<m_reg_sock->errorString();
            break;
        }
        cnter ++;
        if (cnter > 512) break;
    }
    qDebug()<<10<<"try holing ..."<<port<<(port+pnum)<<"done:"<<cnter;    
}

void DtNat::do_phase1()
{
    qDebug()<<"do phase11111...";

    msock = new QUdpSocket();
    msock->bind(QHostAddress::AnyIPv4, 7766);

    stun_buffer tbuf;
    stun_set_binding_request(&tbuf);
    
    QByteArray rdata = QByteArray((char*)tbuf.buf, tbuf.len);
    qint64 bytes;
    QByteArray datagram;
    QHostAddress sender;
    quint16 senderPort;
    QString mapped_addr;

    // F1
    msock->writeDatagram(rdata, QHostAddress(STUN_SERVER_ADDR1), STUN_SERVER_PORT1);
    msock->waitForBytesWritten();

    msock->waitForReadyRead();
    bytes = msock->bytesAvailable();
    qDebug()<<bytes;

    datagram.resize(msock->pendingDatagramSize());
    msock->readDatagram(datagram.data(), datagram.size(),
                       &sender, &senderPort);
    qDebug()<<"read: "<<sender<<senderPort<<datagram.length()<<datagram.toHex();
        
    for (int i = 0; i < datagram.length(); i ++) {
        char c = datagram.at(i);
        fprintf(stderr, "%c", isprint(c) ? c : '.');
        if (i > 375) break;
    }
    fprintf(stderr, " ...]\n");

    mapped_addr = this->getStunAddress(datagram, STUN_ATTRIBUTE_XOR_MAPPED_ADDRESS);
    qDebug()<<"S1 mapped addr:"<<mapped_addr;

    /// F3
    msock->writeDatagram(rdata, QHostAddress(STUN_SERVER_ADDR2), STUN_SERVER_PORT2);
    msock->waitForBytesWritten();

    msock->waitForReadyRead();
    bytes = msock->bytesAvailable();
    qDebug()<<bytes;

    datagram.resize(msock->pendingDatagramSize());
    msock->readDatagram(datagram.data(), datagram.size(),
                       &sender, &senderPort);
    qDebug()<<"read: "<<sender<<senderPort<<datagram.length()<<datagram.toHex();

    for (int i = 0; i < datagram.length(); i ++) {
        char c = datagram.at(i);
        fprintf(stderr, "%c", isprint(c) ? c : '.');
        if (i > 375) break;
    }
    fprintf(stderr, " ...]\n");

    mapped_addr = this->getStunAddress(datagram, STUN_ATTRIBUTE_XOR_MAPPED_ADDRESS);
    qDebug()<<"S2 mapped addr:"<<mapped_addr;
    
    
}

void DtNat::do_phase2()
{

}

void DtNat::do_phase3()
{

}


QString DtNat::getStunAddress(QByteArray resp, uint16_t attr_type)
{
    stun_buffer buf;
    u08bits addr_buff[STUN_BUFFER_SIZE] = {0};

    buf.len = resp.length();
    memcpy(buf.buf, resp.data(), resp.length());

    uint16_t t_attr_type;
    const u08bits *attr_value;
    int attr_len;
    ioa_addr stun_addr;
    stun_attr_ref raw_attr = stun_attr_get_first_str(buf.buf, buf.len);
    for ( ; raw_attr ; raw_attr = stun_attr_get_next_str(buf.buf, buf.len, raw_attr)) {
        t_attr_type = stun_attr_get_type(raw_attr);
        attr_value = stun_attr_get_value(raw_attr);
        attr_len = stun_attr_get_len(raw_attr);

        if (t_attr_type == attr_type) {
            stun_attr_get_addr_str(buf.buf, buf.len, raw_attr, &stun_addr, NULL);
            addr_to_string(&stun_addr, (u08bits*)addr_buff);
            break;
        }
    }

    return QString((char*)addr_buff);
}

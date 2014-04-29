
#include "ns_turn_defs.h"
#include "ns_turn_msg_defs.h"
#include "ns_turn_msg.h"
#include "ns_turn_msg_addr.h"
#include "ns_turn_ioaddr.h"
#include "ns_turn_utils.h"

#include "stun_buffer.h"

#include "dtnat.h"

DtNat::DtNat()
    : QObject(0)
{
    
}

DtNat::~DtNat()
{
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
    // QHostAddress addr("77.72.174.163"); // stun.voxgratia.org // XXX,低版本协议rfc3889
    // QHostAddress addr(""); // stun.voxalot.com        //xxx,低版本协议rfc3889
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


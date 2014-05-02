#include "ns_turn_defs.h"
#include "ns_turn_msg_defs.h"
#include "ns_turn_msg.h"
#include "ns_turn_msg_addr.h"
#include "ns_turn_ioaddr.h"
#include "ns_turn_utils.h"

#include "stun_buffer.h"

#include "xshdefs.h"
#include "stunclient.h"

StunClient::StunClient(quint16 port)
    : QObject()
{
    m_stun_sock = new QUdpSocket();
    QObject::connect(m_stun_sock, &QUdpSocket::connected, this, &StunClient::onStunConnected);
    QObject::connect(m_stun_sock, &QUdpSocket::readyRead, this, &StunClient::onStunReadyRead);

    // 
    Q_ASSERT(port >= STUN_CLIENT_PORT);
    m_stun_port = port;
    m_stun_sock->bind(QHostAddress(STUN_CLIENT_ADDR), port);
}

StunClient::~StunClient()
{
}

bool StunClient::getMappedAddress()
{
    stun_buffer tbuf;
    stun_set_binding_request_str(tbuf.buf, (size_t*)(&(tbuf.len)));

    quint64 wlen2 = m_stun_sock->writeDatagram(QByteArray((char*)tbuf.buf, tbuf.len), 
                                               QHostAddress(STUN_SERVER_ADDR), STUN_SERVER_PORT);    

    return true;
}

bool StunClient::allocate()
{
    stun_buffer alloc_buff;
    stun_set_allocate_request_str(alloc_buff.buf, &alloc_buff.len, 60 * 30, 
                                  STUN_ATTRIBUTE_REQUESTED_ADDRESS_FAMILY_VALUE_IPV4,
                                  STUN_ATTRIBUTE_TRANSPORT_UDP_VALUE,
                                  STUN_ATTRIBUTE_MOBILITY_EVENT);

    QByteArray data = QByteArray((char*)alloc_buff.buf, alloc_buff.len);
    m_stun_sock->write(data);

    return true;
}

void StunClient::onStunConnected()
{

}

void StunClient::onStunReadyRead()
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

void StunClient::processResponse(QByteArray resp)
{
    u08bits rbuf[STUN_BUFFER_SIZE];
    size_t rlen = 0;
    stun_buffer buf;
    QString mapped_addr;
    u08bits addr_buff[STUN_BUFFER_SIZE] = {0};

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
                        addr_to_string(&reflexive_addr, addr_buff);
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

    // emit got addr
    if (strlen((char*)addr_buff) > 0) {
        mapped_addr = QString((char*)addr_buff);
        emit this->mappedAddressRecieved(mapped_addr);
    }
}

void StunClient::onRetryTimeout()
{

}


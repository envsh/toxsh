#include "ns_turn_defs.h"
#include "ns_turn_msg_defs.h"
#include "ns_turn_msg.h"
#include "ns_turn_msg_addr.h"
#include "ns_turn_ioaddr.h"
#include "ns_turn_utils.h"

#include "stun_buffer.h"

#include "stunauth.h"
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

    qint64 wlen2 = m_stun_sock->writeDatagram(QByteArray((char*)tbuf.buf, tbuf.len), 
                                               QHostAddress(STUN_SERVER_ADDR), STUN_SERVER_PORT);    

    return true;
}

bool StunClient::allocate(char *realm, char *nonce)
{
    stun_buffer alloc_buff;
    stun_set_allocate_request_str(alloc_buff.buf, &alloc_buff.len, 60 * 30, 
                                  STUN_ATTRIBUTE_REQUESTED_ADDRESS_FAMILY_VALUE_IPV4,
                                  STUN_ATTRIBUTE_TRANSPORT_UDP_VALUE,
                                  STUN_ATTRIBUTE_MOBILITY_EVENT);

    stun_attr_add_str(alloc_buff.buf, &alloc_buff.len, STUN_ATTRIBUTE_USERNAME, (u08bits*)STUN_USERNAME, strlen(STUN_USERNAME));
    // stun_attr_add_str(alloc_buff.buf, &alloc_buff.len, OLD_STUN_ATTRIBUTE_PASSWORD, (u08bits*)STUN_PASSWORD, strlen(STUN_PASSWORD));

    if (realm != NULL && nonce != NULL) {
        stun_attr_add(&alloc_buff, STUN_ATTRIBUTE_REALM, realm, strlen(realm));
        stun_attr_add(&alloc_buff, STUN_ATTRIBUTE_NONCE, nonce, strlen(nonce));
        stun_attr_add_integrity_by_user_str(alloc_buff.buf, &alloc_buff.len,
                                            (u08bits*)STUN_USERNAME, (u08bits*)realm, (u08bits*)STUN_PASSWORD,
                                            (u08bits*)nonce, SHATYPE_SHA1);
    }

    QByteArray data = QByteArray((char*)alloc_buff.buf, alloc_buff.len);
    qint64 ret = m_stun_sock->writeDatagram(data, QHostAddress(STUN_SERVER_ADDR), STUN_SERVER_PORT);
    qDebug()<<ret;

    return true;
}

bool StunClient::createPermission(QString peer_addr)
{
    qDebug()<<peer_addr;
    m_peer_addr = peer_addr;

    stun_buffer buf;


    return true;
}

bool StunClient::sendIndication(QByteArray data)
{

    return true;
}

bool StunClient::channelBind(QString peer_addr)
{
    qDebug()<<peer_addr;
    m_peer_addr = peer_addr;

    Q_ASSERT(!m_realm.isEmpty());
    Q_ASSERT(!m_nonce.isEmpty());

    stun_buffer buf;
    ioa_addr t_peer_addr;
    
    make_ioa_addr_from_full_string((u08bits*)peer_addr.toLatin1().data(), 0, &t_peer_addr);

    stun_set_channel_bind_request(&buf, &t_peer_addr, 0x4000);
    stun_attr_add(&buf, STUN_ATTRIBUTE_USERNAME, STUN_USERNAME, strlen(STUN_USERNAME));
    stun_attr_add(&buf, STUN_ATTRIBUTE_REALM, m_realm.data(), m_realm.length());
    stun_attr_add(&buf, STUN_ATTRIBUTE_NONCE, m_nonce.data(), m_nonce.length());
    stun_attr_add_integrity_by_user_str(buf.buf, &buf.len, (u08bits*)STUN_USERNAME,
                                        (u08bits*)m_realm.data(), (u08bits*)STUN_PASSWORD,
                                        (u08bits*)m_nonce.data(), SHATYPE_SHA1);

    qint64 ret = m_stun_sock->writeDatagram(QByteArray((char*)buf.buf, buf.len), QHostAddress(STUN_SERVER_ADDR), STUN_SERVER_PORT);

    qDebug()<<"write:"<<ret<<peer_addr;
    return true;
}

bool StunClient::channelData(QByteArray data)
{
    stun_buffer buf;

    stun_init_channel_message(0x4000, &buf, data.length(), 1);
    memcpy(buf.buf + 4, data.data(), data.length());

    for (int i = 0 ; i < 5; i++) {
        qint64 ret = m_stun_sock->writeDatagram(QByteArray((char*)buf.buf, buf.len), QHostAddress(STUN_SERVER_ADDR), STUN_SERVER_PORT);

        
        // qint64 ret = m_stun_sock->writeDatagram(QByteArray((char*)buf.buf, buf.len), 
        //                        QHostAddress(m_relayed_addr.split(':').at(0)),
        //                         m_relayed_addr.split(':').at(1).toUShort());

        qDebug()<<"write chan data:"<<ret<<data.length()<<buf.len<<m_peer_addr<<m_relayed_addr;
    }

    return true;
}

bool StunClient::refresh()
{
    return true;
}

bool StunClient::sendRelayData(QByteArray data, QString relayed_addr)
{
    stun_buffer buf;

    stun_init_channel_message(0x4000, &buf, data.length(), 1);
    memcpy(buf.buf + 4, data.data(), data.length());

    qint64 ret = m_stun_sock->writeDatagram(QByteArray((char*)buf.buf, buf.len), 
                                            QHostAddress(relayed_addr.split(':').at(0)),
                                            relayed_addr.split(':').at(1).toUShort());

    qDebug()<<"write relayed data:"<<ret<<data.length()<<buf.len<<m_peer_addr<<m_relayed_addr<<relayed_addr;
    
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
            char c = datagram.at(i);
            fprintf(stderr, "%c", isprint(c) ? c : '.');
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

    if (!stun_is_command_message(&buf)) {
        qDebug()<<resp.length()<<("The response is not a STUN message\n");
        return;
    }

    if (!stun_is_response(&buf)) {
        if (stun_is_indication(&buf)) {
            qDebug()<<"indication data:"<<buf.len;
        } else {
            qDebug()<<resp.length()<<("The response is not a reponse message\n");
        }
        return;
    }

    u16bits stun_method;
    u16bits stun_msg_type;
    stun_method = stun_get_method_str(buf.buf, buf.len);
    stun_msg_type = stun_get_msg_type_str(buf.buf, buf.len);
    qDebug()<<"method:"<<stun_method<<",msg type:"<<stun_msg_type;


    if (!stun_is_success_response(&buf)) {
        int err_code = 0;
        u08bits err_msg[1025] = "\0";
        size_t err_msg_size = sizeof(err_msg);
        if (stun_is_error_response(&buf, &err_code, err_msg, err_msg_size)) {
            printf("The response is an error %d (%s)\n", err_code, (char*) err_msg);
        } else {
            printf("The response is an unrecognized error\n");
        }

        
        // test unauth
        u08bits realm[128] = {0};
        u08bits nonce[256] = {0};
        
        if (stun_is_challenge_response_str(buf.buf, buf.len, &err_code, err_msg, err_msg_size, realm, nonce)) {
            qDebug()<<err_code;
            qDebug()<<err_code<<(char*)err_msg<<(char*)realm<<(char*)nonce;

            m_realm = QByteArray((char*)realm);
            m_nonce = QByteArray((char*)nonce);

            if (stun_method == STUN_METHOD_ALLOCATE) {
                this->allocate((char*)realm, (char*)nonce);
            }
            if (stun_method == STUN_METHOD_CHANNEL_BIND) {
                QThread::msleep(100);
                this->channelBind(m_peer_addr);
            }
        }

        return;
    }

    this->debugStunResponse(resp);

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

        // emit got addr
        if (strlen((char*)addr_buff) > 0) {
            mapped_addr = QString((char*)addr_buff);
            emit this->mappedAddressRecieved(mapped_addr);
        }
        return;
    } // end bind resp


    if (stun_method == STUN_METHOD_ALLOCATE) {
        m_relayed_addr = this->getStunAddress(resp, STUN_ATTRIBUTE_XOR_RELAYED_ADDRESS);
        emit this->allocateDone();
    }

    if (stun_method == STUN_METHOD_CHANNEL_BIND) {
        emit this->channelBindDone(m_relayed_addr);
    }
}

void StunClient::debugStunResponse(QByteArray resp)
{
    u08bits rbuf[STUN_BUFFER_SIZE];
    size_t rlen = 0;
    stun_buffer buf;
    u08bits addr_buff[STUN_BUFFER_SIZE] = {0};

    rlen = resp.length();
    memcpy(rbuf, resp.data(), resp.length());

    buf.len = resp.length();
    memcpy(buf.buf, resp.data(), resp.length());

    if (!stun_is_command_message(&buf)) {
        qDebug()<<resp.length()<<("The response is not a STUN message\n");
        return;
    }

    if (!stun_is_response(&buf)) {
        qDebug()<<resp.length()<<("The response is not a reponse message\n");
        return;
    }

    u16bits stun_method;
    u16bits stun_msg_type;
    stun_method = stun_get_method_str(buf.buf, buf.len);
    stun_msg_type = stun_get_msg_type_str(buf.buf, buf.len);
    qDebug()<<"method:"<<stun_method<<",msg type:"<<stun_msg_type;

    int attr_type;
    const u08bits *attr_value;
    int attr_len;
    char relayed_addr_str[32] = {0};
    char mapped_addr_str[32] = {0};
    ioa_addr relayed_addr;
    ioa_addr mapped_addr;
    uint32_t lifetime = 0;
    uint32_t bandwidth = 0;
    stun_attr_ref raw_attr = stun_attr_get_first_str(buf.buf, buf.len);
    for ( ; raw_attr ; raw_attr = stun_attr_get_next_str(buf.buf, buf.len, raw_attr)) {
        attr_type = stun_attr_get_type(raw_attr);
        attr_value = stun_attr_get_value(raw_attr);
        attr_len = stun_attr_get_len(raw_attr);

        switch (attr_type) {
        case STUN_ATTRIBUTE_SOFTWARE:
            qDebug()<<"attr software:"<<QByteArray((char*)attr_value, attr_len);
            break;
        case STUN_ATTRIBUTE_XOR_MAPPED_ADDRESS:
            stun_attr_get_addr_str(buf.buf, buf.len, raw_attr, &mapped_addr, NULL);
            addr_to_string(&mapped_addr, (u08bits*)mapped_addr_str);
            qDebug()<<"mapped addr:"<<mapped_addr_str;
            break;
        case STUN_ATTRIBUTE_XOR_RELAYED_ADDRESS:
            stun_attr_get_addr_str(buf.buf, buf.len, raw_attr, &relayed_addr, NULL);
            addr_to_string(&relayed_addr, (u08bits*)relayed_addr_str);
            qDebug()<<"relayed addr:"<<relayed_addr_str;
            break;
        case STUN_ATTRIBUTE_MESSAGE_INTEGRITY:
            qDebug()<<"message integrity:"<<attr_len<<QByteArray((char*)attr_value, attr_len).toHex();
            break;
        case STUN_ATTRIBUTE_LIFETIME:
            memcpy(&lifetime, attr_value, attr_len);
            lifetime = nswap32(lifetime);
            qDebug()<<"lifetime:"<<lifetime;
            break;
        case STUN_ATTRIBUTE_BANDWIDTH:
            memcpy(&bandwidth, attr_value, attr_len);
            bandwidth = nswap32(bandwidth);
            qDebug()<<"bandwidth:"<<bandwidth;
            break;
        default:
            qDebug()<<"unkown attr:"<<attr_type<<attr_len;
            break;
        }
    }
}

QString StunClient::getStunAddress(QByteArray resp, uint16_t attr_type)
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

void StunClient::onRetryTimeout()
{

}


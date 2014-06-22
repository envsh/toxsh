#include <QtNetwork>

#include "cpptox.h"

#include "dhtproc.h"

DhtProc::DhtProc()
    : QObject()
{
    m_timer = new QTimer();
    m_timer->setSingleShot(true);
    QObject::connect(m_timer, &QTimer::timeout, this, &DhtProc::process);
}

DhtProc::~DhtProc()
{
}

void DhtProc::start()
{
    qDebug()<<"";

    logger_init("./dhtmon.log", LoggerLevel::INFO);
    IP ip;
    // ip_init(&ip, TOX_ENABLE_IPV6_DEFAULT);
    ip_reset(&ip);
    ip_init(&ip, TOX_ENABLE_IPV6_DEFAULT);

    m_net_core = new_networking(ip, TOX_PORT_DEFAULT);
    m_dht = new_DHT(m_net_core);

    QByteArray hexpubkey = QByteArray((const char *)m_dht->self_public_key, sizeof(m_dht->self_public_key)).toHex();
    qDebug()<<"pubkey:"<<hexpubkey.length()<<hexpubkey;
    emit this->pubkeyDone(hexpubkey);
    
    QString bs_ip = "37.187.20.216";
    QString bs_port = "33445";
    QString bs_pubkey = "4FD54CFD426A338399767E56FD0F44F5E35FA8C38C8E87C8DC3FEAC0160F8E17";

    DHT_bootstrap_from_address(m_dht, bs_ip.toLatin1().data(), TOX_ENABLE_IPV6_DEFAULT,
                               htons(33445),
                               (const uint8_t*) QByteArray::fromHex(bs_pubkey.toLatin1()).data());

    m_timer->start(300);
}

void DhtProc::stop()
{
    
}

void DhtProc::process()
{
    // qDebug()<<"";

    do_DHT(m_dht);

    networking_poll(m_dht->net);

    // qDebug()<<DHT_isconnected(m_dht)<<DHT_size(m_dht);

    if (DHT_isconnected(m_dht) == 0 && qrand() % 15 == 0) {
        QString bs_ip = "37.187.20.216";
        QString bs_port = "33445";
        QString bs_pubkey = "4FD54CFD426A338399767E56FD0F44F5E35FA8C38C8E87C8DC3FEAC0160F8E17";

        DHT_bootstrap_from_address(m_dht, bs_ip.toLatin1().data(), TOX_ENABLE_IPV6_DEFAULT,
                                   htons(33445),
                                   (const uint8_t*) QByteArray::fromHex(bs_pubkey.toLatin1()).data());
    }
    
    int is_connected = DHT_isconnected(m_dht);
    if (is_connected != m_connected) {
        emit this->connected(is_connected);
        m_connected = is_connected;
    }

    int dht_size = DHT_size(m_dht);
    if (dht_size != m_dht_size) {
        emit this->dhtSizeChanged(dht_size);
        m_dht_size = dht_size;
    }

    this->analysis();

    m_timer->start(300);
}

void DhtProc::analysis()
{
    DHT *dht = m_dht;

    /*
    qDebug()<<dht->close_lastgetnodes<<dht->close_bootstrap_times
            <<dht->num_friends;
    */

    QStringList nodes;
    int cc_num = 0;
    for (int i = 0; i < LCLIENT_LIST; i ++) {
        Client_data cd = dht->close_clientlist[i];

        if ((cd.assoc4.timestamp == 0) &&
            (cd.assoc6.timestamp == 0)) {
            continue;
        } 

        QByteArray bin_client_id = QByteArray((const char*)cd.client_id, CLIENT_ID_SIZE);
        QByteArray hex_client_id = bin_client_id.toHex();

        IP_Port addr = cd.assoc4.ip_port;
        QString str_ip = QHostAddress(ntohl(addr.ip.ip4.uint32)).toString();
        unsigned short cli_port = ntohs(addr.port);
        
        nodes << QString("%1:%2").arg(str_ip).arg(cli_port);
        cc_num ++;
        // qDebug()<<cc_num<<str_ip<<cli_port<<"client id:"<<hex_client_id;
    }

    emit this->closeNodes(nodes);
}

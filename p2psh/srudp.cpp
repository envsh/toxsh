
#include "stunclient.h"

#include "srudp.h"

// static
qint64 Srudp::m_pkt_seq = 1;

Srudp::Srudp(StunClient *stun)
    : QObject()
{
    m_stun_client = stun;
    QObject::connect(m_stun_client, &StunClient::packetReceived, this, &Srudp::onRawPacketReceived);
}

Srudp::~Srudp()
{
}

// TODO split big packet > 1000
bool Srudp::sendto(QByteArray data, QString host)
{
    qDebug()<<"need send big packet size:"<<data.length();

    QStringList tsl = host.split(':');
    Q_ASSERT(tsl.size() == 2);

    int cnter = 0;
    int pkt_len = 123;
    QByteArray tba = data;
    while (tba.length() > 0) {
        QThread::msleep(300); // slow download send rate, or stun server maybe think me as DDOS attack.
        this->sendto(tba.left(pkt_len), tsl.at(0), tsl.at(1).toUShort());
        tba = tba.right(tba.length() - pkt_len);
        // if (cnter ++ > 12) break;
    }

    return true;
    // return this->sendto(data, tsl.at(0), tsl.at(1).toUShort());
    // about max udp packet size: https://stackoverflow.com/questions/14993000/the-most-reliable-and-efficient-udp-packet-size
}

bool Srudp::sendto(QByteArray data, QString host, quint16 port)
{
    QJsonObject jobj;

    qint64 cseq = this->m_pkt_seq ++;

    // TODO 更短的key值
    jobj.insert("cmd", CMD_APP);
    jobj.insert("opt", OPT_RELIABLE);
    jobj.insert("reliable", cseq);
    jobj.insert("reliable_ack", 0);
    jobj.insert("unreliable", 0);
    // jobj.insert("stime", (int)time(NULL));

    jobj.insert("id", cseq);
    jobj.insert("pseq", QString("%1").arg(cseq-1));
    jobj.insert("seq", QString("%1").arg(cseq));
    jobj.insert("pkt", QString(data.toHex()));
    // jobj.insert("dlen", QString("%1").arg(data.length()));
    // jobj.insert("host", host);
    // jobj.insert("port", QString("%1").arg(port));
    // jobj.insert("ctime", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    // jobj.insert("mtime", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    jobj.insert("retry", 0);

    this->schedPrepareSendto(QString("%1:%2").arg(m_proto_host).arg(m_proto_port), jobj);

    /*
    m_proto_send_queue.append(jobj);
    QString npkt = QJsonDocument(jobj).toJson(QJsonDocument::Compact);

    qDebug()<<"sending pkt:"<<cseq<<npkt.length()<<m_proto_host<<m_proto_port<<host<<port;

    if (m_client_mode) {
        m_stun_client->replyIndication(QString("%1:%2").arg(m_proto_host).arg(m_proto_port), npkt.toLatin1());
    } else {
        m_stun_client->sendIndication(QString("%1:%2").arg(m_proto_host).arg(m_proto_port), npkt.toLatin1());
    }
    */

    return true;
}

bool Srudp::hasPendingDatagrams()
{
    qint64 nseq = this->m_out_last_pkt_seq + 1;

    if (m_cached_pkt_seqs.contains(nseq)) {
        return true;
    }

    qDebug()<<""<<m_out_last_pkt_seq<<nseq;

    return false;
}
                        
QByteArray Srudp::readDatagram(QHostAddress &addr, quint16 &port)
{
    QByteArray data;
    QJsonObject jobj;
    QString peer_addr;

    jobj = this->getNextPacket();
    if (!jobj.isEmpty()) {
        QByteArray hex_pkt = jobj.value("pkt").toString().toLocal8Bit();
        QByteArray pkt = QByteArray::fromHex(hex_pkt);
        data = pkt;
        peer_addr = jobj.value("peer_addr").toString();
        addr.setAddress(peer_addr.split(':').at(0));
        port = peer_addr.split(':').at(1).toUShort();
    }

    return data;
}


void Srudp::onRawPacketReceived(QByteArray pkt, QString peer_addr)
{
    qDebug()<<(sender())<<pkt.length()<<peer_addr;
    QJsonDocument jdoc = QJsonDocument::fromJson(pkt);
    QJsonObject jobj = jdoc.object();

    if (jobj.isEmpty()) {
        qDebug()<<"any error???"<<peer_addr;
        return;
    }

    // need temp store
    jobj.insert("peer_addr", peer_addr);

    /*
    if (!m_client_mode) {
        m_proto_host = peer_addr.split(':').at(0);
        m_proto_port = peer_addr.split(':').at(1).toUShort();
    }
    */

    // raw rudp packet
    this->rawProtoPacketHandler(jobj);
}

void Srudp::onPacketReceived(QJsonObject jobj)
{
    // handle ack pkt
    if (jobj.value("opt").toInt() & OPT_ACK) {
        int cnter = 0;
        for (int i = 0; i < m_proto_sched_queue.size(); i ++) {
            if (m_proto_sched_queue.at(i).value("reliable") == jobj.value("reliable")) {
                m_proto_sched_queue.remove(i);
                qDebug()<<"remove acked pkt:"<<jobj.value("reliable");
                cnter += 1;
                break;
            }
        }
        if (cnter == 0) {
            qDebug()<<"ack pkt, but already removed from queue."<<jobj.value("reliable");
        }
        if (m_proto_sched_queue.isEmpty()) {
            this->schedNextSendto();
        }
        return;
    }

    // handle send ack
    {
        QJsonObject jobj_ack;
        jobj_ack.insert("cmd", jobj.value("cmd"));
        jobj_ack.insert("opt", jobj.value("opt").toInt() | OPT_ACK);
        jobj_ack.insert("reliable", jobj.value("reliable"));
        jobj_ack.insert("reliable_ack", jobj.value("reliable"));
        jobj_ack.insert("unreliable", 0);
        jobj_ack.insert("stime", (int)time(NULL));

        QString data = QJsonDocument(jobj_ack).toJson(QJsonDocument::Compact);
        if (m_client_mode) {
            m_stun_client->replyIndication(QString("%1:%2").arg(m_proto_host).arg(m_proto_port), data.toLatin1());
        } else {
            m_stun_client->sendIndication(QString("%1:%2").arg(m_proto_host).arg(m_proto_port), data.toLatin1());
        }
        qDebug()<<"sent ACK:"<<jobj.value("seq")<<m_proto_host<<m_proto_port;
    }

    // 
    int pkt_id = jobj.value("id").toInt();
    QString str_pkt_id = pkt_id == 0 ? jobj.value("id").toString() : QString("%1").arg(pkt_id);
    QString pkt_seq = jobj.value("seq").toString();

    QByteArray hex_pkt = jobj.value("pkt").toString().toLocal8Bit();
    QByteArray pkt = QByteArray::fromHex(hex_pkt);

    qDebug()<<"CBR got pkt_id:"<<str_pkt_id<<pkt_seq<<pkt.length()<<"Bytes"<<m_cached_pkt_seqs.size();


    if (this->m_traned_seqs.contains(jobj.value("seq").toString())) {
        qDebug()<<"already traned pkg:"<<str_pkt_id<<pkt_seq<<pkt.length()<<"Bytes";
        return;
    }

    {
        // if (this->verifyPacket(jobj)) {
            this->cachePacket(jobj);
            if (this->m_cached_pkt_seqs.size() > 5) {
                qDebug()<<"ready writing to user client..."<<this->m_cached_pkt_seqs.size();
            }

            if (this->hasPendingDatagrams()) {
                emit this->readyRead();
            } else {
                // find the lost pkt seq
                qDebug()<<"maybe has lost packet:"<<m_in_last_pkt_seq<<m_out_last_pkt_seq;
            }
    }
}

/*
bool Srudp::verifyPacket(QJsonObject jobj)
{
    int seq = jobj.value("seq").toString().toInt();
    if (seq == this->m_in_last_pkt_seq + 1) {
        return true;
    }
    return false;
}
*/
bool Srudp::cachePacket(QJsonObject jobj)
{
    int seq = jobj.value("seq").toString().toInt();
    this->m_traned_seqs[jobj.value("seq").toString()] = 1;
    this->m_cached_pkt_seqs[seq] = jobj;
    
    if (seq == this->m_in_last_pkt_seq + 1) {
        m_in_last_pkt_seq += 1;
    }

    return true;
}

QJsonObject Srudp::getNextPacket()
{
    QJsonObject jobj, jobj2;
    qint64 seq, nseq;
    qint64 rmkey;

    nseq = m_out_last_pkt_seq + 1;
    QList<qint64> keys = this->m_cached_pkt_seqs.keys();
    for (qint64 key : keys) {
        jobj2 = this->m_cached_pkt_seqs[key];
        seq = jobj2.value("seq").toString().toInt();

        if (seq == nseq) {
            rmkey = key;
            jobj = jobj2;
            break;
        }
    }

    if (!jobj.isEmpty()) {
        m_out_last_pkt_seq = nseq;
        this->m_cached_pkt_seqs.remove(rmkey);
    }

    return jobj;
}

void Srudp::guessLostPacket()
{
    QJsonObject jobj, jobj2;
    int seq;
    qint64 rmkey;

    QList<qint64> keys = this->m_cached_pkt_seqs.keys();
    qSort(keys);
    for (qint64 key : keys) {
        jobj2 = this->m_cached_pkt_seqs[key];
        seq = jobj2.value("seq").toString().toInt();
        if (seq > this->m_in_last_pkt_seq) {
            rmkey = key;
            jobj = jobj2;
            break;
        }
    }

    if (!jobj.isEmpty()) {
        // qDebug()<<QString("id(%1,%2), seq(%3,%4)").arg(this->m_last_pkt_id).arg(jobj.value("id").toString())
        //  .arg(this->m_in_last_pkt_seq).arg(jobj.value("seq").toString());
        this->retranLostPacket(this->m_in_last_pkt_seq+1, this->m_last_pkt_id, jobj.value("id").toString());
    } else {
        qDebug()<<QString("id>%d, seq>%2").arg(this->m_last_pkt_id).arg(this->m_in_last_pkt_seq);
    }
}

void Srudp::retranLostPacket(qint64 lost_seq, QString from_id, QString to_id)
{
    /*
    QString url = QString("http://webtim.duapp.com/phpcomet/rtcomet.php?ct=retran&lseq=%1&fid=%2&tid=%3&flow=spush")
        .arg(lost_seq).arg(from_id).arg(to_id);
    if (this->m_retran_urls.contains(url)) {
        if (qrand() % 10 != 9) {
            return;
        }
    }

    if (this->m_nam == NULL) {
        this->m_nam = new QNetworkAccessManager();
        QObject::connect(m_nam, &QNetworkAccessManager::finished, this, &Srudp::onRetranLostPacketFinished);
    }

    qDebug()<<"retran:"<<url;
    this->m_retran_urls[url] = 1;
    QNetworkRequest req(url);
    QNetworkReply *reply = this->m_nam->get(req);
    */
}

void Srudp::onRetranLostPacketFinished(QNetworkReply *reply)
{
    qDebug()<<""<<reply->error()<<reply->errorString();
    reply->deleteLater();
}

////////////
////
///////////
// should call by stun peerA 
// maybe connect has problem, omit connect for test
/*
bool Srudp::connectToHost(QString host, quint16 port)
{
    this->setClientMode(true);
    this->setHost(host, port);

    QJsonObject jobj;
    jobj.insert("cmd", CMD_CONN_REQ);
    jobj.insert("opt", OPT_RELIABLE);
    jobj.insert("reliable", m_pkt_seq ++);
    jobj.insert("unreliable", 0);
    jobj.insert("reliable_ack", 0);
    jobj.insert("stime", (int)time(NULL));

    m_proto_send_queue.append(jobj);
    QString data = QJsonDocument(jobj).toJson(QJsonDocument::Compact);
    m_stun_client->replyIndication(QString("%1:%2").arg(m_proto_host).arg(m_proto_port), data.toLatin1());

    return true;
}
*/
/*
bool Srudp::serverConnectToHost(QString host, quint16 port)
{
    this->setHost(host, port);
    return true;
}
*/

bool Srudp::setHost(QString host, quint16 port)
{
    this->m_proto_host = host;
    this->m_proto_port = port;

    if (!m_proto_send_confirm_timer) {
        m_proto_send_confirm_timer = new QTimer();
        QObject::connect(m_proto_send_confirm_timer, &QTimer::timeout, this, &Srudp::onSendConfirmTimeout);
    }

    if (!m_proto_send_confirm_timer->isActive()) {
        m_proto_send_confirm_timer->start(1000 * 2);
    }

    return true;
}

bool Srudp::setClientMode(bool is_client)
{
    m_client_mode = is_client;
    return true;
}

/*
bool Srudp::disconnectFromHost()
{
    // do nothing
    QJsonObject jobj;
    jobj.insert("cmd", CMD_CLOSE);
    jobj.insert("opt", OPT_NONE);
    jobj.insert("reliable", m_pkt_seq ++);
    jobj.insert("unreliable", 0);
    jobj.insert("reliable_ack", 0);
    jobj.insert("stime", (int)time(NULL));

    m_proto_send_queue.append(jobj);
    QString data = QJsonDocument(jobj).toJson(QJsonDocument::Compact);
    if (m_client_mode) {
        m_stun_client->replyIndication(QString("%1:%2").arg(m_proto_host).arg(m_proto_port), data.toLatin1());
    } else {
        m_stun_client->sendIndication(QString("%1:%2").arg(m_proto_host).arg(m_proto_port), data.toLatin1());
    }

    if (m_proto_send_confirm_timer) {
        m_proto_send_confirm_timer->stop();
    }

    return true;
}
*/

bool Srudp::ping()
{
    QJsonObject jobj;
    jobj.insert("cmd", CMD_PING);
    jobj.insert("opt", OPT_NONE);
    jobj.insert("reliable", 0);
    jobj.insert("unreliable", 0);
    jobj.insert("reliable_ack", 0);
    jobj.insert("data", (int)time(NULL));
    jobj.insert("stime", (int)time(NULL));

    QString data = QJsonDocument(jobj).toJson(QJsonDocument::Compact);
    if (m_client_mode) {
        m_stun_client->replyIndication(QString("%1:%2").arg(m_proto_host).arg(m_proto_port), data.toLatin1());
    } else {
        m_stun_client->sendIndication(QString("%1:%2").arg(m_proto_host).arg(m_proto_port), data.toLatin1());
    }

    return true;    
}

bool Srudp::rawProtoPacketHandler(QJsonObject jobj)
{
    int cmd = jobj.value("cmd").toInt();
    int opt = jobj.value("opt").toInt();
    int reliable = jobj.value("reliable").toInt();
    int reliable_ack = jobj.value("reliable_ack").toInt();
    
    switch (cmd) {
    case CMD_CONN_REQ:
        qDebug()<<"Not supported command:"<<cmd;
        // this->protoConnectHandler(jobj);
        break;
    case CMD_CONN_RSP:
        qDebug()<<"Not supported command:"<<cmd;
        // this->protoConnectedHandler(jobj);
        break;
    case CMD_CLOSE:
        qDebug()<<"Not supported command:"<<cmd;
        // this->protoCloseHandler(jobj);
        break;
    case CMD_PING:
        this->protoPingHandler(jobj);
        break;
    case CMD_PONG:
        this->protoPongHandler(jobj);
        break;
    case CMD_NOOP:
        break;
    case CMD_APP:
        this->onPacketReceived(jobj);
        break;
    default:
        qDebug()<<"unknown cmd:"<<cmd<<jobj;
        break;
    };
    
    return true;
}

// 三次握手完全实现！！！
/*
bool Srudp::protoConnectHandler(QJsonObject jobj)
{
    qDebug()<<"";

    if (jobj.value("opt").toInt() & OPT_ACK) {
        for (int i = 0; i < m_proto_send_queue.size(); i ++) {
            if (m_proto_send_queue.at(i).value("reliable") == jobj.value("reliable")) {
                m_proto_send_queue.remove(i);
                break;
            }
        }
    } else {
        this->m_in_last_pkt_seq = jobj.value("opt").toInt();
        this->m_out_last_pkt_seq = this->m_in_last_pkt_seq;

        jobj.insert("opt", jobj.value("opt").toInt() | OPT_ACK);
        jobj.insert("reliable_ack", jobj.value("reliable").toInt() + 1);
        jobj.insert("stime", (int)time(NULL));

        // 想当于回包不加到发送队列中
        QString data = QJsonDocument(jobj).toJson(QJsonDocument::Compact);
        m_stun_client->sendIndication(QString("%1:%2").arg(m_proto_host).arg(m_proto_port), data.toLatin1());
        // maybe rudp client not recv

        // 
        jobj = QJsonObject();
        jobj.insert("cmd", CMD_CONN_RSP);
        jobj.insert("opt", OPT_RELIABLE);
        jobj.insert("reliable", m_pkt_seq++);
        jobj.insert("reliable_ack", 0);
        jobj.insert("unreilable", 0);
        jobj.insert("stime", (int)time(NULL));


        m_proto_send_queue.append(jobj);        
        data = QJsonDocument(jobj).toJson(QJsonDocument::Compact);
        m_stun_client->sendIndication(QString("%1:%2").arg(m_proto_host).arg(m_proto_port), data.toLatin1());
        
    }
    return true;
}
*/

/*
bool Srudp::protoConnectedHandler(QJsonObject jobj)
{
    qDebug()<<"";

    if (jobj.value("opt").toInt() & OPT_ACK) {
        for (int i = 0; i < m_proto_send_queue.size(); i ++) {
            if (m_proto_send_queue.at(i).value("reliable") == jobj.value("reliable")) {
                m_proto_send_queue.remove(i);
                break;
            }
        }

        // server connected
        emit connected();
    } else {
        this->m_in_last_pkt_seq = jobj.value("opt").toInt();
        this->m_out_last_pkt_seq = this->m_in_last_pkt_seq;

        jobj.insert("opt", jobj.value("opt").toInt() | OPT_ACK);
        jobj.insert("reliable_ack", jobj.value("reliable").toInt() + 1);
        jobj.insert("stime", (int)time(NULL));

        // 想当于回包不加到发送队列中
        QString data = QJsonDocument(jobj).toJson(QJsonDocument::Compact);
        m_stun_client->replyIndication(QString("%1:%2").arg(m_proto_host).arg(m_proto_port), data.toLatin1());

        // client connected
        emit connected(); // client connected
    }

    // do sth. useful

    return true;
}
*/

bool Srudp::protoPingHandler(QJsonObject jobj)
{
    qDebug()<<"";

    jobj.insert("cmd", CMD_PONG);
    jobj.insert("reliable_ack", jobj.value("reliable").toInt() + 1);
    jobj.insert("data", (int)time(NULL));
    jobj.insert("stime", (int)time(NULL));

    // 想当于回复包，不加到发送队列中

    QString data = QJsonDocument(jobj).toJson(QJsonDocument::Compact);
    if (m_client_mode) {
        m_stun_client->replyIndication(QString("%1:%2").arg(m_proto_host).arg(m_proto_port), data.toLatin1());
    } else {
        m_stun_client->sendIndication(QString("%1:%2").arg(m_proto_host).arg(m_proto_port), data.toLatin1());
    }

    return true;
}

bool Srudp::protoPongHandler(QJsonObject jobj)
{
    qDebug()<<jobj.value("reliable")<<jobj.value("reliable_ack")<<(jobj.value("opt").toInt() & OPT_ACK);

    for (int i = 0; i < m_proto_sched_queue.size(); i ++) {
        if (m_proto_sched_queue.at(i).value("reliable") == jobj.value("reliable")) {
            m_proto_sched_queue.remove(i);
            break;
        }
    }
    
    return true;
}

/*
bool Srudp::protoCloseHandler(QJsonObject jobj)
{
    qDebug()<<"";

    // clean sth.

    return true;
}
*/

// TODO need keepalive packet
void Srudp::onSendConfirmTimeout()
{
    int count = m_proto_sched_queue.size();
    bool plog = count == 0 && (qrand() % 20 == 0);

    if (plog) qDebug()<<"there are some packet to confirm: "<<count;

    QVector<qint64> retransmit_seqs;
    bool has_retransmitted = false;
    bool has_ping = false;
    QJsonObject jobj;
    int ntime = time(NULL);
    int rtcnt = 0;
    for (int i = 0; i < m_proto_sched_queue.size(); i ++) {
        if (ntime - m_proto_sched_queue.at(i).value("stime").toInt() > 1) {
            rtcnt ++;
            m_proto_sched_queue[i].insert("stime", ntime);
            m_proto_sched_queue[i].insert("opt", m_proto_sched_queue[i].value("opt").toInt() | OPT_RETRANSMITED);
            m_proto_sched_queue[i].insert("retry", m_proto_sched_queue[i].value("retry").toInt() + 1);

            jobj = m_proto_sched_queue[i];
            if (jobj.value("cmd").toInt() == CMD_PING) {
                has_ping = true;
            }
            if (m_proto_sched_queue.at(i).value("cmd").toInt() == CMD_CONN_REQ) {
                m_proto_sched_queue.remove(i);
                emit this->connectError();

                if (jobj.value("retry").toInt() > 3) {
                } else {
                    qDebug()<<"retry rudp connect...";
                    // has_retransmitted = true;
                    // QString data = QJsonDocument(jobj).toJson(QJsonDocument::Compact);
                    // m_stun_client->sendRelayData(data.toLatin1(), QString("%1:%2").arg(m_proto_host).arg(m_proto_port));
                }
                break;
            } else {
                QThread::msleep(300);
                retransmit_seqs.append(jobj.value("reliable").toInt());
                has_retransmitted = true;

                
                this->schedRealSendto(jobj);

                /*
                QString data = QJsonDocument(jobj).toJson(QJsonDocument::Compact);
                if (m_client_mode) {
                    m_stun_client->replyIndication(QString("%1:%2").arg(m_proto_host).arg(m_proto_port), data.toLatin1());
                } else {
                    m_stun_client->sendIndication(QString("%1:%2").arg(m_proto_host).arg(m_proto_port), data.toLatin1());
                }
                */
            }
        }
    }
    if (plog || has_retransmitted) {
        if (retransmit_seqs.size() > 8) {
            retransmit_seqs.resize(8);
        }
        qDebug()<<"retransmitted pkts:"<<rtcnt<<count<<has_retransmitted<<retransmit_seqs;
    }

    QDateTime nowtime = QDateTime::currentDateTime();
    if (m_proto_last_ping_time.msecsTo(nowtime) >= m_proto_ping_max_timeout) {
        if (!has_retransmitted && !has_ping) {
            m_proto_last_ping_time = nowtime;
            if (m_client_mode) this->ping();
        }
    }
}

bool Srudp::schedPrepareSendto(QString peer_addr, QJsonObject jobj)
{

    jobj.insert("peer_addr", peer_addr);
    m_proto_send_queue.append(jobj);    
    
    this->schedNextSendto();
    
    return true;
}

bool Srudp::schedNextSendto()
{
    if (m_proto_sched_queue.isEmpty()) {
        int t_sched_count = this->m_proto_sched_queue_max_size;
        int sched_count = qMin(t_sched_count, m_proto_send_queue.size());
        
        for (int i = 0; i < sched_count; i++) {
            m_proto_sched_queue.append(m_proto_send_queue.at(i));
        }

        for (int i = sched_count - 1; i >= 0; i--) {
            m_proto_send_queue.remove(i);
        }

        for (int i = 0; i < sched_count; i ++) {
            this->schedRealSendto(m_proto_sched_queue.at(i));
        }
    }
    
    return true;
}

bool Srudp::schedRealSendto(QJsonObject jobj)
{
    QString peer_addr = jobj.value("peer_addr").toString();
    jobj.remove("peer_addr");

    QString npkt = QJsonDocument(jobj).toJson(QJsonDocument::Compact);

    qint64 cseq = jobj.value("reliable").toInt();
    QString host = peer_addr.split(':').at(0);
    quint16 port = peer_addr.split(':').at(1).toUShort();
    qDebug()<<"sending pkt:"<<cseq<<npkt.length()<<m_proto_host<<m_proto_port<<host<<port;

    if (m_client_mode) {
        m_stun_client->replyIndication(peer_addr, npkt.toLatin1());
    } else {
        m_stun_client->sendIndication(peer_addr, npkt.toLatin1());
    }
    

    return true;
}


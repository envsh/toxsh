
#include "stunclient.h"

#include "srudp.h"

// static
qint64 Srudp::m_pkt_seq = 1;

Srudp::Srudp(StunClient *stun)
    : QObject()
{
    m_stun_client = stun;
    QObject::connect(m_stun_client, &StunClient::packetRecieved, this, &Srudp::onRawPacketRecieved);
}

Srudp::~Srudp()
{
}

// TODO split big packet > 1000
bool Srudp::sendto(QByteArray data, QString host)
{
    QStringList tsl = host.split(':');
    Q_ASSERT(tsl.size() == 2);

    int pkt_len = 123;
    QByteArray tba = data;
    while (tba.length() > 0) {
        this->sendto(tba.left(pkt_len), tsl.at(0), tsl.at(1).toUShort());
        tba = tba.right(tba.length() - pkt_len);
    }

    return true;
    // return this->sendto(data, tsl.at(0), tsl.at(1).toUShort());
}

bool Srudp::sendto(QByteArray data, QString host, quint16 port)
{
    QJsonObject jobj;

    qint64 cseq = this->m_pkt_seq ++;

    jobj.insert("cmd", CMD_APP);
    jobj.insert("opt", OPT_RELIABLE);
    jobj.insert("reliable", cseq);
    jobj.insert("reliable_ack", 0);
    jobj.insert("unreliable", 0);
    jobj.insert("stime", (int)time(NULL));

    jobj.insert("id", cseq);
    jobj.insert("pseq", QString("%1").arg(cseq-1));
    jobj.insert("seq", QString("%1").arg(cseq));
    jobj.insert("pkt", QString(data.toHex()));
    jobj.insert("dlen", QString("%1").arg(data.length()));
    jobj.insert("host", host);
    jobj.insert("port", QString("%1").arg(port));
    jobj.insert("ctime", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    jobj.insert("mtime", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    jobj.insert("retry", 0);
    
    this->m_out_caches[cseq] = jobj;

    m_proto_send_queue.append(jobj);
    QString npkt = QJsonDocument(jobj).toJson(QJsonDocument::Compact);

    qDebug()<<"sending pkt:"<<cseq;
    m_stun_client->sendRelayData(npkt.toLatin1(), QString("%1:%2").arg(host).arg(port));

    // if (cseq == 1) {
    if (false) {
        // first pkt, send more times;
        for (int i = 0; i < 2; i++) {
            m_stun_client->sendRelayData(npkt.toLatin1(), QString("%1:%2").arg(host).arg(port));            
        }
    }

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

    jobj = this->getNextPacket();
    if (!jobj.isEmpty()) {
        QByteArray hex_pkt = jobj.value("pkt").toString().toLocal8Bit();
        QByteArray pkt = QByteArray::fromHex(hex_pkt);
        data = pkt;
    }

    return data;
}


void Srudp::onRawPacketRecieved(QByteArray pkt)
{
    qDebug()<<(sender())<<pkt.length();
    QJsonDocument jdoc = QJsonDocument::fromJson(pkt);
    QJsonObject jobj = jdoc.object();

    // raw rudp packet
    this->rawProtoPacketHandler(jobj);
}

void Srudp::onPacketRecieved(QJsonObject jobj)
{
    // handle ack pkt
    if (jobj.value("opt").toInt() & OPT_ACK) {
        for (int i = 0; i < m_proto_send_queue.size(); i ++) {
            if (m_proto_send_queue.at(i).value("reliable") == jobj.value("reliable")) {
                m_proto_send_queue.remove(i);
                qDebug()<<"remove acked pkt:"<<jobj.value("reliable");
                break;
            }
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
        m_stun_client->sendRelayData(data.toLatin1(), QString("%1:%2").arg(m_proto_host).arg(m_proto_port));
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
        if (this->verifyPacket(jobj)) {
            this->cachePacket(jobj);
            if (this->m_cached_pkt_seqs.size() > 5) {
                qDebug()<<"ready writing to user client..."<<this->m_cached_pkt_seqs.size();
            }

            emit this->readyRead();
        } else {
            // find the lost pkt seq
        }
    }
}

bool Srudp::verifyPacket(QJsonObject jobj)
{
    int seq = jobj.value("seq").toString().toInt();
    if (seq == this->m_in_last_pkt_seq + 1) {
        return true;
    }
    return false;
}

bool Srudp::cachePacket(QJsonObject jobj)
{
    int seq = jobj.value("seq").toString().toInt();
    this->m_traned_seqs[jobj.value("seq").toString()] = 1;
    m_in_last_pkt_seq += 1;
    this->m_cached_pkt_seqs[seq] = jobj;
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
bool Srudp::connectToHost(QString host, quint16 port)
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

    QJsonObject jobj;
    jobj.insert("cmd", CMD_CONN_REQ);
    jobj.insert("opt", OPT_RELIABLE);
    jobj.insert("reliable", m_pkt_seq ++);
    jobj.insert("unreliable", 0);
    jobj.insert("reliable_ack", 0);
    jobj.insert("stime", (int)time(NULL));

    m_proto_send_queue.append(jobj);
    QString data = QJsonDocument(jobj).toJson(QJsonDocument::Compact);
    m_stun_client->sendRelayData(data.toLatin1(), QString("%1:%2").arg(m_proto_host).arg(m_proto_port));

    return true;
}

bool Srudp::serverConnectToHost(QString host, quint16 port)
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

bool Srudp::setHost(QString host, quint16 port)
{
    this->m_proto_host = host;
    this->m_proto_port = port;

    return true;
}

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
    m_stun_client->sendRelayData(data.toLatin1(), QString("%1:%2").arg(m_proto_host).arg(m_proto_port));

    if (m_proto_send_confirm_timer) {
        m_proto_send_confirm_timer->stop();
    }

    return true;
}

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

    m_proto_send_queue.append(jobj);
    QString data = QJsonDocument(jobj).toJson(QJsonDocument::Compact);
    m_stun_client->sendRelayData(data.toLatin1(), QString("%1:%2").arg(m_proto_host).arg(m_proto_port));

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
        this->protoConnectHandler(jobj);
        break;
    case CMD_CONN_RSP:
        this->protoConnectedHandler(jobj);
        break;
    case CMD_CLOSE:
        this->protoCloseHandler(jobj);
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
        this->onPacketRecieved(jobj);
        break;
    default:
        break;
    };
    
    return true;
}

// 三次握手完全实现！！！
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
        m_stun_client->sendRelayData(data.toLatin1(), QString("%1:%2").arg(m_proto_host).arg(m_proto_port));
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
        m_stun_client->sendRelayData(data.toLatin1(), QString("%1:%2").arg(m_proto_host).arg(m_proto_port));
        
    }
    return true;
}

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
        m_stun_client->sendRelayData(data.toLatin1(), QString("%1:%2").arg(m_proto_host).arg(m_proto_port));

        // client connected
        emit connected(); // client connected
    }

    // do sth. useful

    return true;
}

bool Srudp::protoPingHandler(QJsonObject jobj)
{
    qDebug()<<"";

    jobj.insert("cmd", CMD_PONG);
    jobj.insert("reliable_ack", jobj.value("reliable").toInt() + 1);
    jobj.insert("data", (int)time(NULL));
    jobj.insert("stime", (int)time(NULL));

    // 想当于回复包，不加到发送队列中

    QString data = QJsonDocument(jobj).toJson(QJsonDocument::Compact);
    m_stun_client->sendRelayData(data.toLatin1(), QString("%1:%2").arg(m_proto_host).arg(m_proto_port));

    return true;
}

bool Srudp::protoPongHandler(QJsonObject jobj)
{
    qDebug()<<jobj.value("reliable")<<jobj.value("reliable_ack")<<(jobj.value("opt").toInt() & OPT_ACK);

    for (int i = 0; i < m_proto_send_queue.size(); i ++) {
        if (m_proto_send_queue.at(i).value("reliable") == jobj.value("reliable")) {
            m_proto_send_queue.remove(i);
            break;
        }
    }
    
    return true;
}

bool Srudp::protoCloseHandler(QJsonObject jobj)
{
    qDebug()<<"";

    // clean sth.

    return true;
}

// TODO need keepalive packet
void Srudp::onSendConfirmTimeout()
{
    int count = m_proto_send_queue.size();
    bool plog = count == 0 && (qrand() % 20 == 0);

    if (plog) qDebug()<<"there are some packet to confirm: "<<count;

    bool has_retransmitted = false;
    bool has_ping = false;
    QJsonObject jobj;
    int ntime = time(NULL);
    int rtcnt = 0;
    for (int i = 0; i < m_proto_send_queue.size(); i ++) {
        if (ntime - m_proto_send_queue.at(i).value("stime").toInt() > 1) {
            rtcnt ++;
            m_proto_send_queue[i].insert("stime", ntime);
            m_proto_send_queue[i].insert("opt", m_proto_send_queue[i].value("opt").toInt() | OPT_RETRANSMITED);
            m_proto_send_queue[i].insert("retry", m_proto_send_queue[i].value("retry").toInt() + 1);

            jobj = m_proto_send_queue[i];
            if (jobj.value("cmd").toInt() == CMD_PING) {
                has_ping = true;
            }
            if (m_proto_send_queue.at(i).value("cmd").toInt() == CMD_CONN_REQ) {
                m_proto_send_queue.remove(i);
                emit this->connectError();
                break;
            } else {
                has_retransmitted = true;
                QString data = QJsonDocument(jobj).toJson(QJsonDocument::Compact);
                m_stun_client->sendRelayData(data.toLatin1(), QString("%1:%2").arg(m_proto_host).arg(m_proto_port));
            }
        }
    }
    if (plog || has_retransmitted) {
        qDebug()<<"retransmitted pkts:"<<rtcnt<<count<<has_retransmitted;
    }

    QDateTime nowtime = QDateTime::currentDateTime();
    if (m_proto_last_ping_time.msecsTo(nowtime) >= m_proto_ping_max_timeout) {
        if (!has_retransmitted && !has_ping) {
            m_proto_last_ping_time = nowtime;
            this->ping();
        }
    }
}


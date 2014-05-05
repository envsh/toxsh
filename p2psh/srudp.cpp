
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

bool Srudp::sendto(QByteArray data, QString host)
{
    QStringList tsl = host.split(':');
    Q_ASSERT(tsl.size() == 2);
    return this->sendto(data, tsl.at(0), tsl.at(1).toUShort());
}

bool Srudp::sendto(QByteArray data, QString host, quint16 port)
{
    QJsonObject jobj;

    qint64 cseq = this->m_pkt_seq ++;
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

    QJsonDocument jdoc(jobj);
    QString npkt = jdoc.toJson(QJsonDocument::Compact);

    qDebug()<<"sending pkt:"<<cseq;
    m_stun_client->sendRelayData(npkt.toLatin1(), QString("%1:%2").arg(host).arg(port));

    // if (cseq == 1) {
    if (true) {
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

    // TODO jobj validation
    

    this->onPacketRecieved(jobj);
}

void Srudp::onPacketRecieved(QJsonObject jobj)
{

    int pkt_id = jobj.value("id").toInt();
    QString str_pkt_id = pkt_id == 0 ? jobj.value("id").toString() : QString("%1").arg(pkt_id);
    QString pkt_seq = jobj.value("seq").toString();

    QByteArray hex_pkt = jobj.value("pkt").toString().toLocal8Bit();
    QByteArray pkt = QByteArray::fromHex(hex_pkt);

    qDebug()<<"CBR got pkt_id:"<<str_pkt_id<<pkt_seq<<pkt.length()<<"Bytes";


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


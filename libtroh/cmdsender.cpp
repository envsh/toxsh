#include "cmdsender.h"

// static 
qint64 CmdSender::m_pkt_seq = 10000 + qrand() % 10000;

CmdSender::CmdSender(int type)
    : QObject(0), m_type(type)
{
    this->init(type);
}

CmdSender::~CmdSender()
{
}

bool CmdSender::init(int type)
{
    if (!m_nam) {
        m_nam = new QNetworkAccessManager();
        QObject::connect(m_nam, &QNetworkAccessManager::finished, this, &CmdSender::onRequestFinished);
    }
    return true;
}

void CmdSender::onNewOutput(QString output, QString cmdid)
{
    QString url = "http://webtim.duapp.com/pmp/pmp.php";
    QNetworkRequest req(url);
    req.setRawHeader("Content-Type", "application/x-www-form-urlencoded;charset=UTF-8");

    QString sql = "INSERT INTO output (cid, lines) VALUES (:cid, :lines)";
    // $params = array(':cid' => $cid, ':lines' => implode('', $comout));

    QJsonObject jobj, jobj2;
    jobj2.insert(":cid", QJsonValue(cmdid));
    jobj2.insert(":lines", QJsonValue(output));

    jobj.insert("sql", QJsonValue(sql));
    jobj.insert("params", jobj2);

    QJsonDocument jdoc(jobj);
    QString command = jdoc.toJson(QJsonDocument::Compact);

    QString data = QString("command=")+command;

    // qDebug()<<"post resp data:"<<data;
    QNetworkReply *rep = this->m_nam->post(req, data.toLocal8Bit()); // must toLocal8Bit()
    
}

void CmdSender::onNetworkError(QNetworkReply::NetworkError code)
{
    QNetworkReply *reply = (QNetworkReply*)(sender());
    qDebug()<<code<<reply->errorString();
}

void CmdSender::onRequestFinished(QNetworkReply *reply)
{
    // qDebug()<<reply->rawHeaderList()<<reply->error()<<reply->errorString();
    qDebug()<<"sent: "<<QUrl(reply->url()).query();

    this->m_mutex.lock();
    this->m_requesting = false;
    if (this->m_resps.size() > 0) {
        this->m_requesting = true;
        QByteArray data = this->m_resps.dequeue();
        this->sendRequest(data);
    }
    this->m_mutex.unlock();

    reply->deleteLater();
}

void CmdSender::onPacketRecieved(QByteArray pkt)
{
    QByteArray data = QByteArray("pkt=") + pkt.toHex().toPercentEncoding()
        + QByteArray("&seq=") + QString("%1").arg(this->m_pkt_seq++).toLocal8Bit();


    // qDebug()<<"post resp data:"<<data;
    // 先发起不一定先发送完成
    // qDebug()<<"sending..."<<url;
    // QNetworkReply *rep = this->m_nam->post(req, data); // must toLocal8Bit()

    this->m_mutex.lock();
    this->m_resps.enqueue(data);

    if (this->m_requesting) {
    } else {
        this->m_requesting = true;
        data = this->m_resps.dequeue();
        this->sendRequest(data);
    }
    this->m_mutex.unlock();
}

void CmdSender::sendRequest(QByteArray data)
{
    // QString url = QString("http://webtim.duapp.com/phpcomet/rtcomet.php?ct=cpush&len=%1").arg(data.length());
    QString url = QString("http://webtim.duapp.com/phpcomet/rtcomet.php?ct=%1&len=%2&seq=%3")
        .arg(this->m_type == CST_CPUSH ? "cpush" : "spush")
        .arg(data.length())
        .arg(this->m_pkt_seq);

    QNetworkRequest req(url);
    req.setRawHeader("Content-Type", "application/x-www-form-urlencoded;charset=UTF-8");
    qDebug()<<"sending..."<<QUrl(url).query();
    QNetworkReply *rep = this->m_nam->post(req, data); // must toLocal8Bit()
    QObject::connect(rep, SIGNAL(error(QNetworkReply::NetworkError)),
                     this, SLOT(onNetworkError(QNetworkReply::NetworkError)));
}


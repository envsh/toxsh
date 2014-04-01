#include <assert.h>

#include "cmdsender.h"

// static 
qint64 CmdSender::m_pkt_seq = 1;

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
    // qDebug()<<"sent: "<<reply->url().query();
    if (reply->error() == QNetworkReply::RemoteHostClosedError
        || reply->error() == 0) {
    } else {
        // maybe need rerequest
        QNetworkRequest req = reply->request();
        QByteArray data = req.attribute(QNetworkRequest::User).toByteArray();
        int retry_times = req.attribute((QNetworkRequest::Attribute)(QNetworkRequest::User+1)).toInt();
        req.setAttribute((QNetworkRequest::Attribute)(QNetworkRequest::User+1), retry_times+1);

        qDebug()<<"maybe need rerequest:" << reply->error() <<reply->errorString()
                << reply->url().query() << reply->rawHeaderPairs() << data.length();
        QNetworkReply *new_reply = this->m_nam->post(req, data);
        QObject::connect(new_reply, SIGNAL(error(QNetworkReply::NetworkError)),
                         this, SLOT(onNetworkError(QNetworkReply::NetworkError)));
        reply->deleteLater();
    }

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
    int pkt_seq_pos = data.indexOf("&seq=");
    assert(pkt_seq_pos != -1);
    QString pkt_seq =data.right(data.length() - pkt_seq_pos - 5);
    // QString url = QString("http://webtim.duapp.com/phpcomet/rtcomet.php?ct=cpush&len=%1").arg(data.length());
    QString url = QString("http://webtim.duapp.com/phpcomet/rtcomet.php?ct=%1&len=%2&seq=%3")
        .arg(this->m_type == CST_CPUSH ? "cpush" : "spush")
        .arg(data.length())
        .arg(pkt_seq);

    QNetworkRequest req(url);
    req.setRawHeader("Content-Type", "application/x-www-form-urlencoded;charset=UTF-8");
    req.setAttribute(QNetworkRequest::User, data);
    req.setAttribute((QNetworkRequest::Attribute)(QNetworkRequest::User+1), QVariant(1));

    //qDebug()<<"sending..."<<QUrl(url).query();
    QNetworkReply *rep = this->m_nam->post(req, data); // must toLocal8Bit()
    QObject::connect(rep, SIGNAL(error(QNetworkReply::NetworkError)),
                     this, SLOT(onNetworkError(QNetworkReply::NetworkError)));
}

void CmdSender::onResetState()
{
    qDebug()<<"curr pkt seq:"<<this->m_pkt_seq;
    this->m_pkt_seq = 1;
}


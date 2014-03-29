#include "cmdresponser.h"

CmdResponser::CmdResponser()
    : QObject(0)
{
    this->init();
}

CmdResponser::~CmdResponser()
{
}

bool CmdResponser::init()
{
    if (!m_nam) {
        m_nam = new QNetworkAccessManager();
        QObject::connect(m_nam, &QNetworkAccessManager::finished, this, &CmdResponser::onRequestFinished);
    }
    return true;
}

void CmdResponser::onNewOutput(QString output, QString cmdid)
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

void CmdResponser::onRequestFinished(QNetworkReply *reply)
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
}

void CmdResponser::onPacketRecieved(QByteArray pkt)
{

    QByteArray data = QByteArray("pkt=") + pkt.toHex().toPercentEncoding();


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

void CmdResponser::sendRequest(QByteArray data)
{
    QString url = QString("http://webtim.duapp.com/phpcomet/rtcomet.php?ct=spush&len=%1").arg(data.length());
    QNetworkRequest req(url);
    req.setRawHeader("Content-Type", "application/x-www-form-urlencoded;charset=UTF-8");
    qDebug()<<"sending..."<<QUrl(url).query();
    QNetworkReply *rep = this->m_nam->post(req, data); // must toLocal8Bit()
}


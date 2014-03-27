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
    qDebug()<<reply->rawHeaderList()<<reply->error()<<reply->errorString();
}



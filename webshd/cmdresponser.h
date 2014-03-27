#ifndef _CMDRESPONSER_H_
#define _CMDRESPONSER_H_

#include <QtCore>
#include <QtNetwork>

class CmdResponser : public QObject
{
    Q_OBJECT;
public:
    CmdResponser();
    virtual ~CmdResponser();
    bool init();
                           
public slots:
    void onNewOutput(QString output, QString cmdid);
    void onRequestFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_nam = NULL;
};


#endif /* _CMDRESPONSER_H_ */

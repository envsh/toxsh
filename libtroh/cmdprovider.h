#ifndef _CMDPROVIDER_H_
#define _CMDPROVIDER_H_


#include <QtCore>
#include <QtNetwork>

class CmdProvider : public QObject
{
    Q_OBJECT;
public:
    CmdProvider(int type);
    virtual ~CmdProvider();
    
    bool init(int type);

    enum { CPT_UNKNOWN = 0, CPT_CPULL, CPT_SPULL};

public slots:
    void onCometClientReadyRead();
    void onCometClientConnected();
    void onCometClientDisconnected();
    void onCometClientError(QAbstractSocket::SocketError socketError);
    void onCometClientHostFound();
    void onCometClientStateChanged(QAbstractSocket::SocketState socketState);

signals:
    void newCommand(QJsonObject jcmd);

private:
    bool connectToCometServer();
    void enqueuePacket(QString pkt_str);
    void parsePacket(QString str);
    void resetCometState();

private:
    int m_type = CPT_UNKNOWN;
    QTcpSocket *m_cmd_recv_comet_client = NULL;

    QString m_pkt_header;
    QString m_pkt_buf;
    bool m_body_found = false;
    QQueue<QJsonObject> m_q_cmds;

    qint64 m_last_pkt_seq = 0;
    QMap<qint64, QJsonObject> m_cached_pkt_seqs; // 缓存非正常顺序到达的包编号，等待后续包到了之后再发送出去
    
};

#endif /* _CMDPROVIDER_H_ */

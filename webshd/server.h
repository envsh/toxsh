#ifndef _SERVER_H_
#define _SERVER_H_

#include <QtCore>

class CmdRunner;

class Server : public QObject
{
    Q_OBJECT;
public:
    Server();
    virtual ~Server();

    bool init();

public slots:
    void onStdoutReady(QString out);
    void onStderrReady(QString out);

signals:
    void newCommand(QString cmd);

private:
    CmdRunner *m_runner = NULL;
};

#endif /* _SERVER_H_ */

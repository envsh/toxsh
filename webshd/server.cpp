#include "server.h"

#include "cmdrunner.h"
#include "cmdprovider.h"
#include "cmdresponser.h"


Server::Server()
    : QObject(0)
{
}

Server::~Server()
{
}

bool Server::init()
{
    m_responser = new CmdResponser();

    m_runner = new CmdRunner();
    QObject::connect(m_runner, &CmdRunner::stdoutReady, m_responser, &CmdResponser::onNewOutput);

    m_runner->init();
    m_runner->start();


    m_provider = new CmdProvider();
    m_provider->init();

    QObject::connect(m_provider, &CmdProvider::newCommand, this, &Server::newCommand);
    QObject::connect(this, &Server::newCommand, m_runner, &CmdRunner::onNewCommand);

    return true;
}

void Server::onStdoutReady(QString out)
{
}

void Server::onStderrReady(QString out)
{

}



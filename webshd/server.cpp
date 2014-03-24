#include "server.h"

#include "cmdrunner.h"

Server::Server()
    : QObject(0)
{
}

Server::~Server()
{
}

bool Server::init()
{
    m_runner = new CmdRunner();
    m_runner->init();

    m_runner->start();

    return true;
}

void Server::onStdoutReady(QString out)
{
}

void Server::onStderrReady(QString out)
{

}



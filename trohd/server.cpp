#include "server.h"

#include "virttcpd.h"
#include "cmdsender.h"

Server::Server()
    : QObject(0)
{
}

Server::~Server()
{
}

bool Server::init()
{
    this->m_sender = new CmdSender();
    this->m_sender->init();

    this->m_vtcpd = new VirtTcpD();

    QObject::connect(this->m_vtcpd, &VirtTcpD::newPacket, this->m_sender, &CmdSender::onPacketRecieved);
    this->m_vtcpd->init();

    return true;
}


void Server::onStdoutReady(QString out)
{
}

void Server::onStderrReady(QString out)
{

}

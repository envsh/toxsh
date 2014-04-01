#include "server.h"

#include "virttcpd.h"
#include "cmdsender.h"
#include "cmdprovider.h"

Server::Server()
    : QObject(0)
{
}

Server::~Server()
{
}

bool Server::init()
{
    this->m_sender = new CmdSender(CmdSender::CST_CPUSH);
    this->m_sender->init(CmdSender::CST_CPUSH);

    this->m_vtcpd = new VirtTcpD();
    this->m_provider = new CmdProvider(CmdProvider::CPT_CPULL);

    QObject::connect(this->m_vtcpd, &VirtTcpD::newPacket, this->m_sender, &CmdSender::onPacketRecieved);
    QObject::connect(this->m_vtcpd, &VirtTcpD::resetSenderState, this->m_sender, &CmdSender::onResetState);
    QObject::connect(this->m_provider, &CmdProvider::newCommand, this->m_vtcpd, &VirtTcpD::onPacketRecieved);


    this->m_vtcpd->init();
    this->m_provider->init(CmdProvider::CPT_CPULL);

    return true;
}


void Server::onStdoutReady(QString out)
{
}

void Server::onStderrReady(QString out)
{

}

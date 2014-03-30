#include "server.h"

#include "cmdprovider.h"
#include "cmdsender.h"
#include "virttcpc.h"

Server::Server()
    : QObject(0)
{
}

Server::~Server()
{
}

bool Server::init()
{
    m_vtcpc = new VirtTcpC();
    m_provider = new CmdProvider(CmdProvider::CPT_SPULL);
    m_sender = new CmdSender(CmdSender::CST_SPUSH);

    QObject::connect(m_provider, &CmdProvider::newCommand, m_vtcpc, &VirtTcpC::onPacketRecieved);
    QObject::connect(m_vtcpc, &VirtTcpC::newPacket, m_sender, &CmdSender::onPacketRecieved);

    // 注意这几个对象的初始化顺序
    m_sender->init(CmdSender::CST_SPUSH);
    m_vtcpc->init();
    m_provider->init(CmdProvider::CPT_SPULL);

    /*
    m_provider = new CmdProvider();
    m_provider->init();

    QObject::connect(m_provider, &CmdProvider::newCommand, this, &Server::newCommand);
    */
    return true;
}

void Server::onStdoutReady(QString out)
{
}

void Server::onStderrReady(QString out)
{

}



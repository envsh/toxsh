#include "server.h"

#include "cmdrunner.h"
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
    m_provider = new CmdProvider();
    m_sender = new CmdSender(CmdSender::CST_SPUSH);

    QObject::connect(m_provider, &CmdProvider::newCommand, m_vtcpc, &VirtTcpC::onPacketRecieved);
    QObject::connect(m_vtcpc, &VirtTcpC::newPacket, m_sender, &CmdSender::onPacketRecieved);

    // 注意这几个对象的初始化顺序
    m_sender->init(CmdSender::CST_SPUSH);
    m_vtcpc->init();
    m_provider->init();

    /*
    m_responser = new CmdResponser();

    m_runner = new CmdRunner();
    QObject::connect(m_runner, &CmdRunner::stdoutReady, m_responser, &CmdResponser::onNewOutput);

    m_runner->init();
    m_runner->start();


    m_provider = new CmdProvider();
    m_provider->init();

    QObject::connect(m_provider, &CmdProvider::newCommand, this, &Server::newCommand);
    QObject::connect(this, &Server::newCommand, m_runner, &CmdRunner::onNewCommand);
    */
    return true;
}

void Server::onStdoutReady(QString out)
{
}

void Server::onStderrReady(QString out)
{

}




#include "toxnet.h"
#include "srudp.h"

#include "tunneld.h"

Tunneld::Tunneld()
    : QObject(0)
{

}

Tunneld::~Tunneld()
{

}

void Tunneld::init()
{
    m_net = new ToxNet;
    m_rudp = new Srudp(m_net);

    
}

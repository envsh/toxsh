#ifndef _XSHCLI_H_
#define _XSHCLI_H_

#include <QtCore>
#include <QtNetwork>

#include "peersrv.h"

class StunClient;


class XshCli : public PeerSrv
{
    Q_OBJECT;
public:
    XshCli();
    virtual ~XshCli();

private:
    
};

#endif /* _XSHCLI_H_ */

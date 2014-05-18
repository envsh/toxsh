#include <stdlib.h>

#include "debugoutput.h"
#include "dtnat.h"

int main(int argc, char **argv)
{
    qInstallMessageHandler(myMessageOutput);
    QCoreApplication app(argc, argv);

    if (argc >= 2 && strcmp(argv[1], "server") == 0) {
        DtNatSrv dns;
        // pdn.test();
        // pdn.init();
        dns.do_phase1();
    } else {
        DtNatCli dnc;
        dnc.init();
    }

    return app.exec();
    return 0;
}

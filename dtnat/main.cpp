#include <stdlib.h>

#include "debugoutput.h"
#include "dtnat.h"

int main(int argc, char **argv)
{
    qInstallMessageHandler(myMessageOutput);
    QCoreApplication app(argc, argv);

    DtNat pdn(argv[1]);
    // pdn.test();
    // pdn.init();
    pdn.do_phase1();

    return app.exec();
    return 0;
}

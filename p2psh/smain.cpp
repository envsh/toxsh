#include <stdlib.h>

#include <QtCore>

#include "debugoutput.h"
#include "xshsrv.h"


int main(int argc, char **argv)
{
    qInstallMessageHandler(myMessageOutput);
    QCoreApplication app(argc, argv);

    XshSrv xsh;
    xsh.init();

    return app.exec();
    return 0;
}

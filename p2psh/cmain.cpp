#include <stdlib.h>

#include <QtCore>

#include "debugoutput.h"
#include "xshcli.h"

int main(int argc, char **argv)
{
    qInstallMessageHandler(myMessageOutput);
    QCoreApplication app(argc, argv);

    XshCli cli;
    cli.init();

    return app.exec();
    return 0;
}

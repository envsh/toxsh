#include <stdlib.h>

#include <QtCore>

#include "debugoutput.h"

int main(int argc, char **argv)
{
    qInstallMessageHandler(myMessageOutput);
    QCoreApplication app(argc, argv);


    return app.exec();
    return 0;
}

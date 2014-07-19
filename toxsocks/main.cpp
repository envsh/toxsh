
#include <QCoreApplication>

#include "debugoutput.h"

int main(int argc, char **argv)
{
    qInstallMessageHandler(myMessageOutput);
    QCoreApplication app(argc, argv);

    if (argc < 2) {
        qDebug()<<"tunnel server | client";
    }

    return app.exec();
    return 0;
}

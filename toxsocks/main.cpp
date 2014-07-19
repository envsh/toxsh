
#include <QCoreApplication>

#include "debugoutput.h"

#include "tunneld.h"

int main(int argc, char **argv)
{
    qInstallMessageHandler(myMessageOutput);
    QCoreApplication app(argc, argv);

    if (argc == 2 && strcmp(argv[1], "server") == 0) {
        Tunneld *tund = new Tunneld;
        tund->init();
    } else if (argc == 2 && strcmp(argv[1], "client") == 0) {

    } else {
        qDebug()<<"tunnel server | client";
    } 

    return app.exec();
    return 0;
}

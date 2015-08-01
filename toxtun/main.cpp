
#include <QCoreApplication>

#include "debugoutput.h"

// #include "qtoxkit.h"

#include "tunneld.h"
#include "tunnelc.h"

int main(int argc, char **argv)
{
    qInstallMessageHandler(myMessageOutput);
    QCoreApplication app(argc, argv);

    if (argc == 2 && strcmp(argv[1], "server") == 0) {
        Tunneld *tund = new Tunneld;
        tund->init();
    } else if (argc == 2 && strcmp(argv[1], "client") == 0) {
        Tunnelc *tunc = new Tunnelc;
        tunc->init();
    } else {
        qDebug()<<"tunnel server | client";
    } 

    // QToxKit toxkit;
    // toxkit.start();
    
    return app.exec();
    return 0;
}

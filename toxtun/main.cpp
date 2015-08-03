
#include <QCoreApplication>

#include "debugoutput.h"

// #include "qtoxkit.h"

#include "tunneld.h"
#include "tunnelc.h"

int main(int argc, char **argv)
{
    qInstallMessageHandler(myMessageOutput);
    QCoreApplication app(argc, argv);

    QString config_file = "./toxtun_whttp.ini";
    if (argc >= 3) {
        config_file = QString(argv[2]);
    }
    
    if (argc >= 2 && strcmp(argv[1], "server") == 0) {
        Tunneld *tund = new Tunneld(config_file);
        tund->init();
    } else if (argc >= 2 && strcmp(argv[1], "client") == 0) {
        Tunnelc *tunc = new Tunnelc(config_file);
        tunc->init();
    } else {
        qDebug()<<"tunnel <server | client> [toxtun_xxx.ini]";
    }

    // QToxKit toxkit;
    // toxkit.start();
    
    return app.exec();
    return 0;
}

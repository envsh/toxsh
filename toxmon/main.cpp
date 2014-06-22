
#include "debugoutput.h"
#include "dhtmon.h"

int main(int argc, char **argv)
{
    qInstallMessageHandler(&myMessageOutput);
    QApplication a(argc, argv);

    DhtMon dm;
    dm.show();

    return a.exec();
}

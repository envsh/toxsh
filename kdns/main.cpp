#include <stdlib.h>

#include <QtCore>

#include "debugoutput.h"
#include "kdns.h"

int main(int argc, char**argv)
{
    // setenv("QT_MESSAGE_PATTERN", "[%{type}] %{appname} (%{file}:%{line}) T%{threadid} %{function} - %{message} ", 1);
    qInstallMessageHandler(myMessageOutput);
    
    QCoreApplication a(argc, argv);
    
    KDNS dns;
    dns.init();

    return a.exec();
    return 0;
}


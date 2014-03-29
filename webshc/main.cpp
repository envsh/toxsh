#include <stdlib.h>

#include <QtCore>

// #include "server.h"

int main(int argc, char**argv)
{
    setenv("QT_MESSAGE_PATTERN", "[%{type}] %{appname} (%{file}:%{line}) T%{threadid} %{function} - %{message} ", 1);
    QCoreApplication a(argc, argv);
    
    //  Server s;
    // s.init();

    return a.exec();
    return 0;
}


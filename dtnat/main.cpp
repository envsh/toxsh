#include <stdlib.h>

#include "dtnat.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    DtNat pdn;
    pdn.test();

    return app.exec();
    return 0;
}

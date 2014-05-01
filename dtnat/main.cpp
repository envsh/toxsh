#include <stdlib.h>

#include "dtnat.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    DtNat pdn(argv[1]);
    // pdn.test();
    pdn.init();


    return app.exec();
    return 0;
}

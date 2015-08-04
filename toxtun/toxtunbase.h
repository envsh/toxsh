#ifndef TOXTUNBASE_H
#define TOXTUNBASE_H

#include <QtCore>

class ToxTunBase : public QObject
{
    Q_OBJECT;
    
public:
    uint32_t nextConid();
    
public:
    uint32_t m_conid = 7;
};


#endif /* TOXTUNBASE_H */

#ifndef _DHTMON_H_
#define _DHTMON_H_

#include <QtGui>
#include <QtWidgets>


namespace Ui {
    class DhtMon;
};

class DhtProc;

class DhtMon : public QMainWindow
{
    Q_OBJECT;
public:
    DhtMon();
    virtual ~DhtMon();

public slots:
    void onStart();
    void onStop();

    void onPubkeyDone(QByteArray pubkey);
    void onConnected(int conn);
    void onDhtSizeChanged(int size);

private:
    Ui::DhtMon *m_win = NULL;
    DhtProc *m_proc = NULL;
};

#endif /* _DHTMON_H_ */

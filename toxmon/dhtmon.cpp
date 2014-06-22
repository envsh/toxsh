
#include "dhtproc.h"

#include "ui_dhtmon.h"
#include "dhtmon.h"


DhtMon::DhtMon()
    : QMainWindow()
    , m_win(new Ui::DhtMon)
{
    m_win->setupUi(this);

    QObject::connect(m_win->pushButton, &QPushButton::clicked, this, &DhtMon::onStart);
    QObject::connect(m_win->pushButton_2, &QPushButton::clicked, this, &DhtMon::onStop);
}

DhtMon::~DhtMon()
{
}

void DhtMon::onStart()
{
    m_proc = new DhtProc;
    QObject::connect(m_proc, &DhtProc::pubkeyDone, this, &DhtMon::onPubkeyDone);
    QObject::connect(m_proc, &DhtProc::connected, this, &DhtMon::onConnected);
    QObject::connect(m_proc, &DhtProc::dhtSizeChanged, this, &DhtMon::onDhtSizeChanged);

    m_proc->start();
}

void DhtMon::onStop()
{
    
}

void DhtMon::onPubkeyDone(QByteArray pubkey)
{
    m_win->lineEdit->setText(QString(pubkey));
}

void DhtMon::onConnected(int conn)
{
    m_win->lineEdit_2->setText(QString("%1").arg(conn));
}

void DhtMon::onDhtSizeChanged(int size)
{
    m_win->lineEdit_3->setText(QString("%1").arg(size));
}

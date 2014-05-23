#include "queue.h"


QueueItem::QueueItem()
    : QObject()
{
    m_got4 = m_got6 = false;
}

QueueItem::~QueueItem()
{
}

QString QueueItem::hash()
{
    QString orig = QString("%1:%2'%3").arg(m_addr.toString()).arg(m_port).arg(m_qid);
    return orig;
}

void QueueItem::debug()
{
    qDebug()<<m_addr<<m_port<<m_qid<<m_got4<<m_got6<<m_time;
}

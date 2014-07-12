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

    void onNodeClicked(const QModelIndex &idx);

    // from proc
    void onPubkeyDone(QByteArray pubkey);
    void onConnected(int conn);
    void onDhtSizeChanged(int size);
    void onDhtNodesChanged(int friendCount, int clientCount, int ping_array_size, int harden_ping_array_size);
    void onCloseNodesArrived(const QStringList &nodes);

private:
    Ui::DhtMon *m_win = NULL;
    DhtProc *m_proc = NULL;
    QStringList m_nodes;
    QStringListModel *m_nodes_model = NULL;
    QHash<QString, int> m_pinged; // ping过程的节点列表
};

#endif /* _DHTMON_H_ */

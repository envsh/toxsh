
#include "qtoxkit.h"



class ToxDhtServer
{
public:
    QString addr = "";
    int port = -1;
    QString pubkey;
    QString name;
};


class ToxSettings
{
public:
    ToxSettings(QString identifier = "anon", bool persist = true) {
        m_persist = persist;
        m_ident = identifier;
        
        m_bdir = QString("%1/.config/toxkit").arg(QDir::home().path());
        m_sdir = m_bdir + QString("/%1").arg(m_ident);

        m_qpath = m_bdir + "qtox.ini";
        m_qsets = new QSettings(m_qpath, QSettings::IniFormat);
        m_data = m_sdir + "/tkdata";
        m_friend_sets = new QSettings(m_sdir + "/toxkit.friend.lst", QSettings::IniFormat);

        if (persist) {
            if (!QDir().exists(m_bdir)) {
                QDir().mkdir(m_bdir);
            }
            if (!QDir().exists(m_sdir)) {
                QDir().mkdir(m_sdir);
            }
            if (!QFile::exists(m_qpath)) {
                QString srcfile = QFileInfo(__FILE__).absoluteDir().path() + "/../etc/qtox.ini";
                QFile::copy(srcfile, m_qpath);
                m_qsets = new QSettings(m_qpath, QSettings::IniFormat);
            }
        }
    }
    
    ~ToxSettings() {}

    QVector<ToxDhtServer> getDhtServerList() {
        m_qsets->beginGroup("DHT Server/dhtServerList");
        int stsize = m_qsets->value("size").toInt();

        QVector<ToxDhtServer> dhtsrvs;
        for (int i = 1; i <= stsize; i++) {
            ToxDhtServer dhtsrv;
            dhtsrv.addr = m_qsets->value(QString("%1/address").arg(i)).toString();
            dhtsrv.port = m_qsets->value(QString("%1/port").arg(i)).toInt();
            dhtsrv.pubkey = m_qsets->value(QString("%1/userId").arg(i)).toString();
            dhtsrv.name = m_qsets->value(QString("%1/name").arg(i)).toString();
            dhtsrvs.append(dhtsrv);
        }
        m_qsets->endGroup();
        return dhtsrvs;
    }

    QByteArray getSaveData() {
        if (!m_persist) return QByteArray();

        QFile fp(m_data);
        fp.open(QIODevice::ReadOnly);
        QByteArray data = fp.readAll();
        fp.close();
        return data;
    }

    bool saveData(QByteArray data) {
        if (!m_persist) return true;
        if (data.length() == 0) return false;

        QFile fp(m_data);
        fp.open(QIODevice::ReadWrite | QIODevice::Truncate);
        fp.write(data);
        fp.close();

        return true;
    }

    bool saveFriends(QStringList friends) {
        if (!m_persist) return true;
        
        m_friend_sets->beginGroup("FriendList");
        int fn = friends.count();
        m_friend_sets->setValue("size", QVariant(fn));
        int i = 0;
        foreach (QString fid, friends) {
            m_friend_sets->setValue(QString::number(i++), fid);
        }
        m_friend_sets->endGroup();
        return true;
    }

    QStringList loadFriends() {
        if (!m_persist) return QStringList();

        m_friend_sets->beginGroup("FriendList");
        int fn = m_friend_sets->value("size").toInt();
        
        QStringList friends;
        for (int i = 0; i < fn; i++) {
            QString fid = m_friend_sets->value(QString("%1").arg(i)).toString();
            friends.append(fid);
        }
        qDebug()<<"load friend done:"<<friends.count();
        m_friend_sets->endGroup();

        QStringList hcfriends =
            {"398C8161D038FD328A573FFAA0F5FAAF7FFDE5E8B4350E7D15E6AFD0B993FC529FA90C343627",
             "4610913CF8D2BC6A37C93A680E468060E10335178CA0D23A33B9EABFCDF81A46DF5DDE32954A",
             "2645081363C7E8B5090523098A563D3BE3A6D92227B251E55FE42FBBA277500DC80EF1F7CF4A",};
        
        foreach (QString fid, hcfriends) {
            if (!friends.contains(fid)) friends.append(fid);
        }

        return friends;
    }

    
public:
    bool m_persist = true;
    QString m_ident;
    // $HOME/.config/toxkit/{identifier}/{tkdata,toxkit.friend.lst}
    QString m_bdir;
    QString m_sdir;
    QString m_qpath;

    QSettings* m_qsets;
    QString m_data;
    QSettings* m_friend_sets;

};

QToxKit::QToxKit(QString identifier, bool persist, QObject* parent)
    : QThread(parent)
{
    m_sets = new ToxSettings(identifier, persist);
}

QToxKit::~QToxKit()
{
}


//////////////// callbacks
void on_self_connection_status(Tox *tox, TOX_CONNECTION connection_status, void *user_data)
{
    QToxKit *qtox = (QToxKit*)user_data;
    qDebug()<<tox<<connection_status;


    if (qtox->m_first_connect && connection_status > TOX_CONNECTION_NONE) {
        qtox->m_first_connect = false;
        int fc = tox_self_get_friend_list_size(tox);
        uint32_t fns[20] = {0};
        tox_self_get_friend_list(tox, fns);
        qDebug()<<fc;

        bool bret;
        TOX_ERR_FRIEND_GET_PUBLIC_KEY err;
        uint8_t public_key[TOX_PUBLIC_KEY_SIZE];
        for (int i = 0; i < fc; i++) {
            bret = tox_friend_get_public_key(tox, fns[i], public_key, &err);
            QByteArray hex_pubkey = QByteArray((char*)public_key, TOX_PUBLIC_KEY_SIZE).toHex().toUpper();
            qDebug()<<i<<fns[i]<<bret<<err<<hex_pubkey;
        }
    }
    
    int conn_status = connection_status;
    // emit qtox->selfConnectChanged(connection_status > TOX_CONNECTION_NONE);
    emit qtox->selfConnectionStatus(conn_status);
}

void on_friend_connection_status(Tox *tox, uint32_t friend_number, TOX_CONNECTION connection_status,
                                 void *user_data)
{
    QToxKit *qtox = (QToxKit*)user_data;
    qDebug()<<tox<<connection_status;

    uint8_t public_key[TOX_PUBLIC_KEY_SIZE];
    tox_friend_get_public_key(tox, friend_number, public_key, NULL);
    QString hex_pubkey = QByteArray((char*)public_key, TOX_PUBLIC_KEY_SIZE).toHex().toUpper();

    int conn_status = (int)connection_status;
    emit qtox->friendConnectionStatus(hex_pubkey, conn_status);
}

void on_friend_status(Tox *tox, uint32_t friend_number, TOX_USER_STATUS user_status,
                      void *user_data)
{
    QToxKit *qtox = (QToxKit*)user_data;
    qDebug()<<tox<<user_status;

    uint8_t public_key[TOX_PUBLIC_KEY_SIZE];
    tox_friend_get_public_key(tox, friend_number, public_key, NULL);
    QString hex_pubkey = QByteArray((char*)public_key, TOX_PUBLIC_KEY_SIZE).toHex().toUpper();

    int iuser_status = (int)user_status;
    emit qtox->friendStatus(hex_pubkey, iuser_status);
}

void on_friend_request(Tox *tox, const uint8_t *public_key, const uint8_t *message, size_t length,
                       void *user_data)
{
    QToxKit *qtox = (QToxKit*)user_data;
    
    QString hex_pubkey = QByteArray((char*)public_key, TOX_PUBLIC_KEY_SIZE).toHex().toUpper();
    QString qmessage = QByteArray((char*)message, length);
    qDebug()<<hex_pubkey<<qmessage;

    emit qtox->friendRequest(hex_pubkey, qmessage);
}


void on_friend_message(Tox *tox, uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t *message,
                       size_t length, void *user_data)
{
    QToxKit *qtox = (QToxKit*)user_data;
    // qDebug()<<tox<<friend_number<<length;

    uint8_t public_key[TOX_PUBLIC_KEY_SIZE];
    tox_friend_get_public_key(tox, friend_number, public_key, NULL);
    QString hex_pubkey = QByteArray((char*)public_key, TOX_PUBLIC_KEY_SIZE).toHex().toUpper();

    QByteArray qmessage = QByteArray((char*)message, length);
    emit(qtox->friendMessage(hex_pubkey, type, qmessage));
}


////////////////
static void bootDht(QToxKit *qtox)
{
    Tox *tox = qtox->m_tox;
    bool bret;

    
    const char *ipaddr = "128.199.78.247";
    QByteArray hex_pubkey("34922396155AA49CE6845A2FE34A73208F6FCD6190D981B1DBBC816326F26C6C");
    QByteArray pubkey = QByteArray::fromHex(hex_pubkey);
    // bret = tox_bootstrap(tox, ipaddr, 33445, (uint8_t*)pubkey.data(), NULL);
    // bret = tox_add_tcp_relay(tox, ipaddr, 33445, (uint8_t*)pubkey.data(), NULL);
    qDebug()<<bret<<ipaddr<<hex_pubkey;
}


static void bootLocal(QToxKit *qtox)
{

}


static void makeTox(QToxKit *qtox)
{
    TOX_ERR_OPTIONS_NEW err = TOX_ERR_OPTIONS_NEW_OK;
    struct Tox_Options *opts = tox_options_new(&err);

    QByteArray savedata = qtox->m_sets->getSaveData();
    opts->savedata_type = TOX_SAVEDATA_TYPE_TOX_SAVE;
    opts->savedata_data = (const uint8_t*)savedata.data();
    opts->savedata_length = savedata.length();
    
    TOX_ERR_NEW err2 = TOX_ERR_NEW_OK;
    Tox *tox = tox_new(opts, &err2);
    qtox->m_tox = tox;

    tox_callback_self_connection_status(tox, on_self_connection_status, qtox);
    tox_callback_friend_connection_status(tox, on_friend_connection_status, qtox);
    tox_callback_friend_request(tox, on_friend_request, qtox);
    tox_callback_friend_message(tox, on_friend_message, qtox);
    tox_callback_friend_status(tox, on_friend_status, qtox);

    ////// some tests
    uint8_t toxaddr[TOX_ADDRESS_SIZE];
    tox_self_get_address(tox, toxaddr);
    qDebug()<<"myaddr:"<<QByteArray((char*)toxaddr, TOX_ADDRESS_SIZE).toHex().toUpper();
    
    /*
    auto f = [](Tox *tox, uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t *message,
                size_t length, void *user_data) -> void {
    };
    tox_callback_friend_message(tox, f, qtox);
    */
}

static void itimeout(QToxKit *qtox)
{
    Tox *tox = qtox->m_tox;

    tox_iterate(tox);
}

static void _quick_save_data(QToxKit *toxkit)
{
    int sz = tox_get_savedata_size(toxkit->m_tox);
    uint8_t savedata[sz] = {0};
    tox_get_savedata(toxkit->m_tox, savedata);
    QByteArray qsavedata((char*)savedata, sz);
    toxkit->m_sets->saveData(qsavedata);

}

uint32_t QToxKit::friendAdd(QString friendId, QString requestMessage)
{
    QByteArray raw_friend_id = QByteArray::fromHex(friendId.toLatin1());
    TOX_ERR_FRIEND_ADD err;
    uint32_t friend_number =
        tox_friend_add(m_tox, (uint8_t*)raw_friend_id.data(),
                       (uint8_t*)requestMessage.toLatin1().data(), requestMessage.length(), &err);
    if (friend_number == UINT32_MAX) {
        qDebug()<<"error:"<<err;
    } else {
        _quick_save_data(this);
    }
    return friend_number;
}

uint32_t QToxKit::friendAddNorequest(QString friendId)
{
    QByteArray raw_friend_id = QByteArray::fromHex(friendId.toLatin1());
    TOX_ERR_FRIEND_ADD err;
    uint32_t friend_number = tox_friend_add_norequest(m_tox, (uint8_t*)raw_friend_id.data(), &err);
    if (friend_number == UINT32_MAX) {
        qDebug()<<"error:"<<err;
    } else {
        _quick_save_data(this);
    }
    return friend_number;
}

bool QToxKit::friendDelete(QString friendId)
{
    QByteArray raw_friend_id = QByteArray::fromHex(friendId.toLatin1());
    TOX_ERR_FRIEND_BY_PUBLIC_KEY err0;
    uint32_t friend_number = tox_friend_by_public_key(m_tox, (uint8_t*)raw_friend_id.data(), &err0);
    TOX_ERR_FRIEND_DELETE err;
    bool bret = tox_friend_delete(m_tox, friend_number, &err);
    if (!bret) {
        qDebug()<<"error:"<<err<<friend_number;
    }
    return bret;
}

void QToxKit::friendSendMessage(QString friendId, QByteArray data)
{
    Tox *tox = this->m_tox;

    QByteArray raw_friend_id = QByteArray::fromHex(friendId.toLatin1());
    uint32_t friend_number = tox_friend_by_public_key(tox, (uint8_t*)raw_friend_id.data(), NULL);
    uint32_t msgid = tox_friend_send_message(tox, friend_number, TOX_MESSAGE_TYPE_NORMAL,
                                             (uint8_t*)data.data(), data.length(), NULL);
    // qDebug()<<friend_number<<msgid<<data.length();
}

void QToxKit::run()
{
    makeTox(this);
    bootDht(this);

    Tox *tox = this->m_tox;
    int itval = tox_iteration_interval(tox);

    while (true) {
        itimeout(this);
        QThread::msleep(itval * 1);  // * 9
    }
}


#include "toxtunutils.h"

void ToxTunRecord::dump()
{
    qDebug()<<"begin dump ToxTunRecord:"<<this;
    printf("......local_host:local_port,%s:%d\n", m_local_host.toLatin1().data(), m_local_port);
    printf("......remote_host:remote_port,%s:%d\n", m_remote_host.toLatin1().data(), m_remote_port);
    printf("......remote_pubkey,%s\n", m_remote_pubkey.toLatin1().data());
    qDebug()<<"end dump ToxTunRecord: "<<this;
}


ToxTunConfig::ToxTunConfig(QString config_file)
{
    m_config_file = config_file;
    m_config_sets = new QSettings(config_file, QSettings::IniFormat);
    
    // this->loadHardCoded();
    this->loadConfig();
}

void ToxTunConfig::loadHardCoded()
{
    ToxTunRecord rec;

    rec = ToxTunRecord();
    rec.m_local_host = "*";
    rec.m_local_port = 8115;
    rec.m_remote_host = "127.0.0.1";
    rec.m_remote_port = 8119;
    // rec.m_remote_pubkey = _srvpeer;
    this->m_recs.append(rec);

    rec = ToxTunRecord();
    rec.m_local_host = "*";
    rec.m_local_port = 8181;
    rec.m_remote_host = "127.0.0.1";
    rec.m_remote_port = 81;
    // rec.m_remote_pubkey = _srvpeer;
    // this->m_recs.append(rec);

    rec = ToxTunRecord();
    rec.m_local_host = "*";
    rec.m_local_port = 8282;
    rec.m_remote_host = "127.0.0.1";
    rec.m_remote_port = 82;
    // rec.m_remote_pubkey = _srvpeer;
    // this->m_recs.append(rec);

    /// server信息
    this->m_srvname = "anon";
}

void ToxTunConfig::loadConfig()
{
    QSettings & sets = *m_config_sets;

    sets.beginGroup("client");
    QStringList keys = sets.childKeys();
    foreach(QString key, keys) {
        if (key == "size") continue;
        if (key.startsWith("#")) continue;//   # comment
        
        QString val = sets.value(key).toString();
        QStringList elems = val.split(':');

        // TODO 检测配置格式
        ToxTunRecord rec = ToxTunRecord();
        rec.m_local_host = elems[0]; //  # '*'
        rec.m_local_port = elems[1].toInt(); //  # 8282
        rec.m_remote_host = elems[2]; //  # '127.0.0.1'
        rec.m_remote_port = elems[3].toInt(); //  # 82
        rec.m_remote_pubkey = elems[4]; //   # _srvpeer

        this->m_recs.append(rec);
        // break; //   # just for test, only use one validate config

        sets.endGroup();
        
        // ### server信息
        m_srvname = "whttpd";
        sets.beginGroup("server");
        m_srvname = sets.value("name").toString();
        sets.endGroup();
    }
}



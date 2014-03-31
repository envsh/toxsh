#include <assert.h>
#include <unistd.h>

#include "cmdprovider.h"

CmdProvider::CmdProvider(int type)
    : QObject(0), m_type(type)
{

}

CmdProvider::~CmdProvider()
{
}


bool CmdProvider::init(int type)
{
    this->connectToCometServer();
    return true;
}

bool CmdProvider::connectToCometServer()
{
    if (!m_cmd_recv_comet_client) {
        m_cmd_recv_comet_client = new QTcpSocket();
        QObject::connect(m_cmd_recv_comet_client, &QTcpSocket::connected, this, &CmdProvider::onCometClientConnected);
        QObject::connect(m_cmd_recv_comet_client, &QTcpSocket::disconnected, this, &CmdProvider::onCometClientDisconnected);
        QObject::connect(m_cmd_recv_comet_client, &QTcpSocket::hostFound, this, &CmdProvider::onCometClientHostFound);
        QObject::connect(m_cmd_recv_comet_client, &QTcpSocket::readyRead, this, &CmdProvider::onCometClientReadyRead);
        QObject::connect(m_cmd_recv_comet_client, SIGNAL(error(QAbstractSocket::SocketError)),
                         this, SLOT(onCometClientError(QAbstractSocket::SocketError)));
    }

    // TODO: 解析一次DNS，多次使用的模式，更通用。
    // home: 119.75.219.53
    // other: 202.108.23.200
    m_cmd_recv_comet_client->connectToHost("119.75.219.53", 80);
    // m_cmd_recv_comet_client->connectToHost("127.0.0.1", 80);

    return true;
}

void CmdProvider::onCometClientReadyRead()
{
    QByteArray ba = m_cmd_recv_comet_client->readAll();

    bool debug_output = true;
    QByteArray tmp;
    // debug
    if (ba.length() == 254) {
        // tmp = ba;
        debug_output = false;
        if (qrand() % 10 == 9) {
            debug_output = true;
        }
    }
    // debug
    if (debug_output) {
        if (this->m_type == CPT_CPULL) {
            qDebug()<<"HCPS -> CBR:"<<ba.length()<<"Bytes"<<","<<tmp;
        } else {
            qDebug()<<"HSPS -> SBR:"<<ba.length()<<"Bytes"<<","<<tmp;
        }
    }

    this->parsePacket(QString(ba));
}

void CmdProvider::onCometClientConnected()
{
    // http://webtim.duapp.com/phpcomet/coment.php?ct=s
    // QString hdr = "GET /phpcomet/rtcomet.php?ct=cpull HTTP/1.0\r\n"
    QString hdr = QString("GET /phpcomet/rtcomet.php?ct=%1 HTTP/1.0\r\n"
                          "Host: webtim.duapp.com\r\n"
                          "Accept: */*\r\n"
                          "\r\n")
        .arg(this->m_type == CPT_CPULL ? "cpull" : "spull");

    int rc = m_cmd_recv_comet_client->write(hdr.toLatin1());
}


void CmdProvider::onCometClientDisconnected()
{
    // qDebug()<<"comet connection done. try next.";
    this->resetCometState();
    sleep(1);
    this->connectToCometServer();
}

void CmdProvider::onCometClientError(QAbstractSocket::SocketError socketError)
{
    if (socketError == QAbstractSocket::RemoteHostClosedError) {
    } else {
        qDebug()<<socketError;
    }
}

void CmdProvider::onCometClientHostFound()
{
    // qDebug()<<"hoho";
}

void CmdProvider::onCometClientStateChanged(QAbstractSocket::SocketState socketState)
{
}

/////////////
void CmdProvider::enqueuePacket(QString pkt_str)
{
    QJsonDocument jdoc = QJsonDocument::fromJson(pkt_str.toLocal8Bit());
    // qDebug()<<"pkt type:"<<jdoc.isArray()<<jdoc.isObject()<<jdoc.isEmpty();
    if (jdoc.isEmpty()) {
        qDebug()<<"Error: Invalid pkt str.";
        return;
    }

    QJsonObject jobj = jdoc.object();
    assert(jobj.contains("id") && jobj.contains("pkt"));
    this->m_q_cmds.enqueue(jobj);

    emit newCommand(jobj);
}

void CmdProvider::parsePacket(QString str)
{
    int pkt_len = 5120;
    QString pkt_suffix = "ENDOFPKT\r\n\r\nNP:\n";

    this->m_pkt_buf += str;

    // 解析http header
    if (!this->m_body_found) {
        int bpos = this->m_pkt_buf.indexOf("\r\n\r\n");
        if (bpos > 0) {
            this->m_body_found = true;
            this->m_pkt_header = this->m_pkt_buf.left(bpos + 4);
            this->m_pkt_buf = this->m_pkt_buf.right(this->m_pkt_buf.length() - (bpos + 4));
            // qDebug()<< "header"<<this->m_pkt_header<<"kkk";
            // sleep(3);
        }
    }
    if (this->m_body_found) { 
        // echo "{$this->_pkt_buf}";
        // echo strlen($this->_pkt_buf) . "\n";
        int epos = this->m_pkt_buf.indexOf(pkt_suffix);
        // qDebug()<<"suffix end pos:"<<epos;
        // echo "pkt epos: {$epos}\n";exit;
        if (epos > 0) {
            QString padded_cmd = this->m_pkt_buf.left(epos + pkt_suffix.length());
            QString jcmd = padded_cmd.trimmed();
            for (int i = jcmd.length()-1; i >= 0; i--) {
                if (jcmd.at(i) == QChar('$')) continue;
                jcmd = jcmd.left(i+1);
                break;
            }
            // qDebug()<< "found a cmd: "<<jcmd<<"hhh";
            // static $cmd_id = 1;
            // array_push($this->_q_cmds, array('cmd'=>$cmd, 'id'=>$cmd_id++));
            this->m_pkt_buf = this->m_pkt_buf.right(this->m_pkt_buf.length() - (epos + pkt_suffix.length()));
            this->enqueuePacket(jcmd);
        } else {
                
        }
    }
    
}

void CmdProvider::resetCometState()
{
    this->m_pkt_header.clear();
    this->m_body_found = false;
    this->m_pkt_buf.clear();
    //  assert(this->m_q_cmds.size() == 0);
        
}

    /*
    private function enqueuePacket($pkt_str)
    {
        $pkt_arr = json_decode($pkt_str, true);
        if ($pkt_arr) {
            array_push($this->_q_cmds, $pkt_arr);
        } else {
            echo "decode pkg error: ";
            var_dump($pkt_str);
        }
        
    }

    private function parsePacket($str)
    {
        $pkt_len = 5120;
        $pkt_suffix = "ENDOFPKT\r\n\r\nP:";

        $this->_pkt_buf .= $str;

        // 解析http header
        if (!$this->_body_found) {
            $bpos = strpos($this->_pkt_buf, "\r\n\r\n");
            if ($bpos > 0) {
                $this->_body_found = true;
                $this->_pkt_header = substr($this->_pkt_buf, 0, $bpos + 4);
                $this->_pkt_buf = substr($this->_pkt_buf, $bpos + 4);
                echo "{$this->_pkt_header}kkk\n";
                // sleep(3);
            }
        } else { 
            // echo "{$this->_pkt_buf}";
            // echo strlen($this->_pkt_buf) . "\n";
            $epos = strpos($this->_pkt_buf, $pkt_suffix);
            // var_dump($epos);
            // echo "pkt epos: {$epos}\n";exit;
            if ($epos > 0) {
                $padded_cmd = substr($this->_pkt_buf, 0, $epos - strlen($pkt_suffix));
                $cmd = rtrim($padded_cmd, "\$\n");
                echo "found a cmd: {$cmd}hhh\n";
                // static $cmd_id = 1;
                // array_push($this->_q_cmds, array('cmd'=>$cmd, 'id'=>$cmd_id++));
                $this->_pkt_buf = substr($this->_pkt_buf, $epos + strlen($pkt_suffix));
                $this->enqueuePacket($cmd);
            } else {
                
            }
        }
    }

    private function runNextCommand()
    {
        while (true) {
            if (count($this->_q_cmds) == 0) {
                break;
            }

            assert(count($this->_q_cmds) > 0);
            $cmd = array_pop($this->_q_cmds);
            $cmd['cmd'] = $this->_color_cmd($cmd['cmd']);
            $real_cmd = $cmd['cmd'] . $this->_cmd_suffix;
            $wret = fwrite($this->_pipes[0], $real_cmd);
            echo $cmd['cmd'];
            if ($cmd['cmd'] == 'sudo bash') {
                $this->_send_response(array($this->_cmd_suffix), $cmd['id']);
            } else {
                $this->_read_response($cmd['id']);
            }
            $this->_set_done_state(array($cmd['id']));
        }
    }

    private function resetCometState()
    {
        $this->_pkt_header = '';
        $this->_body_found = false;
        $this->_pkt_buf = '';
        assert(count($this->_q_cmds) == 0);
    }

    */

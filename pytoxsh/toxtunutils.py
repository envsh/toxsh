
import sys, os
from PyQt5.QtCore import *
from srudp import *

_srvpeer = '6BA28AC06C1D57783FE017FA9322D0B356E61404C92155A04F64F3B19C75633E8BDDEFFA4856'
# _srvpeer = '95893EDB96D2E4C97F46BA2C81779A27F3DB84F8C1BE3B47AE2209B78170BE311442A7BCA183'

class ToxTunRecord():
    def __init__(self):
        self.local_host = ''
        self.local_port = 0
        self.remote_host = ''
        self.remote_port = 0
        self.remote_pubkey = ''
        return

    def dump(self):
        qDebug('begin dump ToxTunRecord: %s' % str(self))
        print('......local_host:local_port,', self.local_host, self.local_port)
        print('......remote_host:remote_port,', self.remote_host, self.remote_port)
        print('......remote_pubkey,', self.remote_pubkey)
        qDebug('end dump ToxTunRecord: %s' % str(self))
        return

class ToxTunConfig():
    def __init__(self, config_file):
        self.config_file = config_file
        self.config_sets = QSettings(self.config_file, QSettings.IniFormat)
        
        self.recs = []
        self.srvname = ''

        if os.path.exists(self.config_file) is False:
            qDebug('config file not exists: %s' % config_file)
            sys.exit(-1)
        
        # self.loadHardCoded()
        self.loadConfig()
        return

    def loadHardCoded(self):
        rec = ToxTunRecord()
        rec.local_host = '*'
        rec.local_port = 8115
        rec.remote_host = '127.0.0.1'
        rec.remote_port = 8119
        rec.remote_pubkey = _srvpeer
        self.recs.append(rec)

        rec = ToxTunRecord()
        rec.local_host = '*'
        rec.local_port = 8181
        rec.remote_host = '127.0.0.1'
        rec.remote_port = 81
        rec.remote_pubkey = _srvpeer
        # self.recs.append(rec)

        rec = ToxTunRecord()
        rec.local_host = '*'
        rec.local_port = 8282
        rec.remote_host = '127.0.0.1'
        rec.remote_port = 82
        rec.remote_pubkey = _srvpeer
        # self.recs.append(rec)

        ### server信息
        self.srvname = 'anon'
        
        return

    def loadConfig(self):
        sets = self.config_sets

        sets.beginGroup('client')
        keys = sets.childKeys()
        for key in keys:
            if key == 'size': continue
            if key[0:1] == '#': continue   # comment
            val = sets.value(key)
            elems = val.split(':')

            # TODO 检测配置格式
            
            rec = ToxTunRecord()
            rec.local_host = elems[0]  # '*'
            rec.local_port = int(elems[1])  # 8282
            rec.remote_host = elems[2]  # '127.0.0.1'
            rec.remote_port = int(elems[3])  # 82
            rec.remote_pubkey = elems[4]  # _srvpeer

            self.recs.append(rec)
            break   # just for test, only use one validate config

        
        sets.endGroup()


        ### server信息
        self.srvname = 'whttpd'
        sets.beginGroup('server')
        self.srvname = sets.value('name')
        sets.endGroup()
        
        return

    # 检测配置record是否有效
    def recordValid(self):
        
        return


class ToxTunConst():
    # basic
    pkt_min_size = 128
    pkt_max_size = 512
    pkt_encode_type = 'hex'   # hex/base64

    # advanced const
    pkt_enable_compress = False  # gzip
    pkt_enable_extra_encrypt = False
    pkt_extra_encrypt_method = ''  #
    pkt_timeout = 30000    # ms

    def __init__(self): return


class ToxTunConnection():
    def __init__(self):
        self.peer = None   # 72B的
        self.self_file_number = -1  # for transport via file
        self.peer_file_number = -1  # for transport via file
        self.reqchunks = []
        
        self.srv = None  # QTcpServer
        self.rec = None  # ToxTunRecord
        self.rno = -1    # ToxTunRecord sequence no
        return


# for temporary compact 
class ToxConnection(ToxTunConnection):
    def __init__(self):
        super(ToxConnection, self).__init__()


class ToxTunChannel():
    def __init__(self, con, sock):
        self.con = con
        self.sock = sock  #
        self.host = ''
        self.port = 0
        # self.chano = 0
        # self.cmdno = 0  #
        self.chanocli = -5 
        self.chanosrv = -6   #
        self.rudp = None  # Srudp instance
        self.transport = None  # ToxTunTransport instance

        # extra info, like stats, time, speed
        self.pktnum = 0
        self.rdlen = 0  # about socket
        self.wrlen = 0
        self.tnlen = 0  # toxnet length, calc net work data length
        self.ctime = QDateTime.currentDateTime()
        self.atime = None  # last active time
        self.etime = None  # end time
        self.offline_count = 0       # 在连接生存周期内离线打断次数
        self.offline_times = {}  # offline_no => [starttime, endtime]

        # for close, promise all True for real defer chan obj
        self.peer_close = False
        self.sock_close = False
        self.rudp_close = False

        return

    def debugInfo(self):
        info = 'chano: %d, offcnt: %d, cmdno: %d, pktnum=%d, ' % \
               (self.chano, self.offline_count, 0, self.pktnum)
        return info
    

# for temporary compact
class ToxChannel(ToxTunChannel):
    def __init__(self, con, sock):
        super(ToxChannel, self).__init__(con, sock)


#
class FileReqChunk():
    def __init__(self):
        self.file_number = -1
        self.position = -1
        self.length = -1
        return


#
class ToxTunFileChannel():
    def __init__(self, con, sock):
        super(ToxTunFileChannel, self).__init__()
        self.con = con
        self.sock = sock  #
        self.host = ''
        self.port = 0
        self.chanocli = 0  
        self.chanosrv = 0   #
        # self.self_file_number = -1
        # self.peer_file_number = -1
        # self.reqchunks = []

        # extra info, like stats, time, speed
        self.pktnum = 0
        self.rdlen = 0
        self.wrlen = 0
        self.tnlen = 0  # toxnet length, calc net work data length
        self.ctime = QDateTime.currentDateTime()
        self.atime = None  # last active time
        self.etime = None  # end time
        self.offline_count = 0       # 在连接生存周期内离线打断次数
        self.offline_times = {}  # offline_no => [starttime, endtime]

        # for close, promise all True for real defer chan obj
        self.peer_close = False
        self.ctrl_close = False
        self.sock_close = False
        self.need_sent_close = False
        return


def help():
    print('Usage: python %s [/path/to/config_file.ini]' % sys.argv[0])
    print('Example: python %s ./toxtun.ini' % sys.argv[0])
    print('')
    
    return


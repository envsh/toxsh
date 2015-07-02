
from PyQt5.QtCore import *


# http://blog.csdn.net/yuanrxdu/article/details/21465495
# http://blog.csdn.net/yuanrxdu/article/details/21476105

class SruConst():
    # 连接保持协议
    RUDP_SYN = 0x10  # 主动发起连接
    RUDP_SYN2 = 0x11   # 发起连接返回包
    RUDP_SYN_ACK = 0x02  # SYN2的ACK
    RUDP_FIN = 0x13     # 主动发起关闭
    RUDP_FIN2 = 0x14     # 关闭返回包
    RUDP_KEEPALIVE = 0x15  # 心跳包
    RUDP_KEEPALIVE_ACK = 0x16  # 心跳返回包

    # 数据协议
    RUDP_DATA = 0x20   # 可靠数据
    RUDP_DATA_ACK = 0x23   # 可靠数据确认
    RUDP_DATA_NACK = 0x24   # 丢包确认

    # 网络错误码
    ERROR_SYN_STATE = 0x01
    SYSTEM_SYN_ERROR = 0x02

    RUDP_INIT = 0x0
    RUDP_ESTAB = 0x1
    RUDP_CLOSED = 0x2
    # client状态
    RUDP_SYN_SENT = 0x3


class SruUtils():
    def randISN():
        import random
        return random.randrange(7, pow(2, 17))
    
class SruPacket2():
    def __init__(self):
        self.msg_type = ''
        self.seq = 0
        self.ack = 0
        self.data = ''
        self.extra = ''
        return

    def encode(self):
        pkt = {
            'msgtype': self.msg_type,
            'seq': self.seq,
            'ack': self.ack,
            'data': self.data,
            'extra': self.extra,
        }

        import json
        jspkt = json.JSONEncoder().encode(pkt)
        return jspkt
        return

    def decode(jspkt):
        import json
        pkt = json.JSONDecoder().decode(jspkt)
        
        opkt = SruPacket2()
        opkt.msg_type = pkt['msgtype']
        opkt.seq = pkt['seq']
        opkt.ack = pkt['ack']
        opkt.data = pkt['data']
        opkt.extra = pkt['extra']

        return opkt
    

class RUDPSendSegment():
    def __init__(self):
        self.seq = 0
        self.push_ts = 0
        self.last_sent_ts = 0
        self.send_count = 0
        self.data = ''
        return


class RUDPRecvSegment():
    def __init__(self):
        self.seq = 0
        self.data = ''
        return


class Srudp(QObject):
    connected = pyqtSignal()
    disconnected = pyqtSignal()
    error = pyqtSignal(int)
    readyRead = pyqtSignal()
    bytesWritten = pyqtSignal()
    newConnection = pyqtSignal()

    lossPacket = pyqtSignal()

    def __init__(self, parent = None):
        super(Srudp, self).__init__(parent);
        self.selfisn = 0
        self.peerisn = 0
        self.selfseq = 0
        self.peerseq = 0

        self.state = 'INIT'
        self.rcvpkts = {}  # 接收缓冲区
        self.rcvwins = {}  # 确认窗口，外部可读取的
        self.sndpkts = {}  # 发送缓冲区
        self.sndwins = {}  # 发送窗口
        self.rcvseq = 0  # 确认已经接收的包序号
        self.maxrcvseq = 0  # 接收到的包中最大包序号
        self.rcvlosspkts = []   # 丢失的包序号
        self.sndseq = 0  # 确认已发送的包序号
        
        self.mode = 'SERVER'  # SERVER|CLIENT
        self.pendCons = []    # pending connectoins

        return


    def mkcon(self, extra):
        self.mode = 'CLIENT'
        self.state = 'SYN_SENT'
        opkt = SruPacket2()
        opkt.msg_type = 'SYN'
        opkt.seq = SruUtils.randISN()
        opkt.extra = extra
        
        self.selfisn = opkt.seq
        self.selfseq = opkt.seq + 2
        jspkt = opkt.encode()
        return jspkt

    def mkdiscon(self, extra):

        if self.mode == 'CLIENT':
            self.state = 'FIN1'
        else:
            self.state = 'LAST_ACK'

        opkt = SruPacket2()
        opkt.msg_type = 'FIN1'
        opkt.extra = extra

        jspkt = opkt.encode()
        return jspkt

    def buf_recv_pkt(self, jspkt):
        opkt = SruPacket2.decode(jspkt)

        res = None
        if self.mode == 'SERVER':
            res = self._statemachine_server(opkt)
        elif self.mode == 'CLIENT':
            res = self._statemachine_client(opkt)
        else:
            qDebug('srudp mode error: %s' % self.mode)
        return res

    def buf_send_pkt(self, data, extra):
        opkt = SruPacket2()
        opkt.msg_type = 'DATA'
        opkt.seq = self.selfseq
        self.selfseq += 1
        opkt.data = data
        opkt.extra = extra

        jspkt = opkt.encode()
        return jspkt

    def _statemachine_client(self, opkt):
        if self.state == 'SYN_SENT':
            if opkt.msg_type == 'SYN2' and opkt.ack == (self.selfisn + 1) and opkt.seq > 0:
                self.peerisn = opkt.seq
                self.peerseq = opkt.seq + 2
                self.state = 'ESTAB'
                ropkt = SruPacket2()
                ropkt.msg_type = 'SYN_ACK'
                ropkt.seq = opkt.ack
                ropkt.ack = opkt.seq + 1
                ropkt.extra = opkt.extra
                qDebug('client peer ESTABed')
                self.connected.emit()
                return ropkt.encode()
            else: qDebug('proto error, except SYN_ACK pkt')
        elif self.state == 'ESTAB':
            if opkt.msg_type == 'FIN1':  # 服务器端行发送关闭请求
                ropkt = SruPacket2()
                ropkt.msg_type = 'FIN2'
                ropkt.extra = opkt.extra
                self.state = 'CLOSE_WAIT'
                return ropkt.encode()
            elif opkt.msg_type == 'DATA':
                qDebug('got data: %s' % opkt.encode())
                ropkt = SruPacket2()
                ropkt.msg_type = 'DATA_ACK'
                ropkt.seq = opkt.seq
                ropkt.ack = opkt.seq+1
                ropkt.extra = opkt.extra
                self.rcvpkts[opkt.seq] = opkt
                if opkt.seq > self.maxrcvseq: self.maxrcvseq = opkt.seq
                QTimer.singleShot(1, self._on_recv_data)
                qDebug('hehre')
                return ropkt.encode()
            elif opkt.msg_type == 'DATA_ACK':
                qDebug('acked data.')
            else: qDebug('unexcepted msg type: %s' % opkt.msg_type)
            pass
        elif self.state == 'FIN1':
            if opkt.msg_type == 'FIN2':
                self.state = 'FIN2'
            pass
        elif self.state == 'FIN2':
            if opkt.msg_type == 'FIN1':
                self.state = 'TIME_WAIT'

                ropkt = SruPacket2()
                ropkt.msg_type = 'FIN2'
                ropkt.extra = opkt.extra
                return ropkt.encode()
            pass
        elif self.state == 'TIME_WAIT':
            qDebug('here')
            pass
        elif self.state == 'CLOSE_WAIT':
            qDebug('omited')
            pass
        elif self.state == 'LAST_ACK':
            if opkt.msg_type == 'FIN2':
                self.state = 'CLOSED'
                qDebug('cli peer closed.')
                self.disconnected.emit()
            pass
        elif self.state == 'CLOSED':
            qDebug('here')
            pass
        else: qDebug('proto error: %s ' % self.state)
        
        return

    def _statemachine_server(self, opkt):
        if self.state == 'INIT':
            if opkt.msg_type == 'SYN' and opkt.seq > 0:
                self.state = 'SYN_RCVD'
                self.peerisn = opkt.seq
                self.peerseq = opkt.seq + 2
                ropkt = SruPacket2()
                ropkt.msg_type = 'SYN2'
                ropkt.seq = SruUtils.randISN()
                ropkt.ack = opkt.seq + 1
                ropkt.extra = opkt.extra
                self.selfisn = ropkt.seq
                self.selfseq = ropkt.seq + 2
                return ropkt.encode()
            else: qDebug('proto error: except syn=1')
        elif self.state == 'SYN_RCVD':
            if opkt.msg_type == 'SYN_ACK' and opkt.ack == (self.selfisn + 1) and opkt.seq == (self.peerisn + 1):
                self.state = 'ESTAB'
                qDebug('server peer ESTABed')
                self.newConnection.emit()
                pass
            else: qDebug('proto error: except syn ack')
        elif self.state == 'ESTAB':
            if opkt.msg_type == 'FIN1':
                ropkt = SruPacket2()
                ropkt.msg_type = 'FIN2'
                ropkt.extra = opkt.extra
                self.state = 'CLOSE_WAIT'
                return ropkt.encode()
            elif opkt.msg_type == 'DATA':
                qDebug('got data. %s' % opkt.encode())
                ropkt = SruPacket2()
                ropkt.msg_type = 'DATA_ACK'
                ropkt.seq = opkt.seq
                ropkt.ack = opkt.seq+1
                ropkt.extra = opkt.extra
                self.rcvpkts[opkt.seq] = opkt
                if opkt.seq > self.maxrcvseq: self.maxrcvseq = opkt.seq
                QTimer.singleShot(1, self._on_recv_data)
                qDebug('hehre')
                return ropkt.encode()
            elif opkt.msg_type == 'DATA_ACK':
                qDebug('acked data.')
            else: qDebug('unexcepted msg type: %s' % opkt.msg_type)
            pass
        elif self.state == 'FIN1':
            if opkt.msg_type == 'FIN2':
                self.state = 'FIN2'
            pass
        elif self.state == 'FIN2':
            if opkt.msg_type == 'FIN1':
                self.state = 'TIME_WAIT'

                ropkt = SruPacket2()
                ropkt.msg_type = 'FIN2'
                ropkt.extra = opkt.extra
                return ropkt.encode()
            pass
        elif self.state == 'CLOSE_WAIT':
            qDebug('omited')
            pass
        elif self.state == 'LAST_ACK':
            if opkt.msg_type == 'FIN2':
                self.state = 'CLOSED'
                qDebug('srv peer closed.')
                self.disconnected.emit()
            pass
        elif self.state == 'CLOSED':
            qDebug('already closed connection')
            pass
        else: qDebug('unkown state: %s' % self.state)
        return

    # 接收窗口相关控制，包检测控制
    # 
    def _on_recv_data(self):
        if self.rcvseq == 0: self.rcvseq = self.peerseq

        # 查找loss的包
        loss_pkts = []
        cnter = self.rcvseq
        while cnter < (self.rcvseq + 20) and cnter <= self.maxrcvseq:
            if cnter in self.rcvpkts:
                cnter += 1
                continue
            loss_pkts.append(cnter)
            cnter += 1
        if len(loss_pkts) > 0: self.rcvlosspkts += loss_pkts
        qDebug('loss pkts: %d, peerseq=%d, rcvseq=%d, maxrcvseq=%d' %
               (len(loss_pkts), self.peerseq, self.rcvseq, self.maxrcvseq))
        self.lossPacket.emit()

        
        # 查找readyread的包
        ready_pkts = []
        cnter = self.rcvseq
        while cnter < self.rcvseq + 20 and cnter <= self.maxrcvseq:
            if cnter in self.rcvpkts:
                ready_pkts.append(self.rcvpkts[cnter])
            else: break
            cnter += 1

        qDebug('ready read pkts: %d' % len(ready_pkts))
        if len(ready_pkts) > 0:
            for opkt in ready_pkts:
                self.rcvwins[opkt.seq] = opkt
                self.rcvpkts.pop(opkt.seq)
                if opkt.seq in self.rcvlosspkts: self.rcvlosspkts.remove(opkt.seq)
            self.rcvseq = max(self.rcvwins.keys()) + 1
            self.readyRead.emit()
            
        return

    
    # 发送窗口相关控制，包重发控制
    def _on_sent_data(self):

        return

    def getLossPackets(self):
        return self.rcvlosspkts

    def readPacket(self):
        if len(self.rcvwins) > 0:
            mkey = min(self.rcvwins.keys())
            opkt = self.rcvwins.pop(mkey)
            return opkt
        return None

################
class SrudTransport():
    def __init__(self):
        return


class SrudControl():
    def __init__(self):
        return


###############
class SruPacket():
    # CLIENT_ISN = 0  # random 32b integer
    
    def __init__(self):
        self.seq = 0
        self.syn = 0
        self.ack = 0
        self.fin = 0
        self.chksum = 0
        self.data = ''
        return

    def encode(self):
        pkt = {
            'seq': self.seq,
            'syn': self.syn,
            'ack': self.ack,
            'fin': self.fin,
            'chksum': self.chksum,
            'data': self.data,
        }
        import json
        jspkt = json.JSONEncoder().encode(pkt)
        return jspkt
    
    def decode(jspkt):
        import json
        pkt = json.JSONDecoder().decode(jspkt)
        
        opkt = SruPacket()
        opkt.seq = pkt['seq']
        opkt.syn = pkt['syn']
        opkt.ack = pkt['ack']
        opkt.fin = pkt['fin']
        opkt.chksum = pkt['chksum']
        opkt.data = pkt['data']

        return opkt

    def randISN():
        import random
        return random.randrange(7, pow(2, 17))
    
    def mkhs1pkt():

        opkt = SruPacket()
        opkt.seq = SruPacket.randISN()
        opkt.syn = 1

        return opkt

    def mkhs2pkt(client_isn):
        
        opkt = SruPacket()
        opkt.seq = SruPacket.randISN()
        opkt.syn = 1
        opkt.ack = client_isn + 1

        return opkt

    def mkhs3pkt(client_isn, server_isn):
        opkt = SruPacket()
        opkt.seq = client_isn + 1
        opkt.syn = 0
        opkt.ack = server_isn + 1

        return opkt

    def mkfinpkt():
        opkt = SruPacket()
        opkt.fin = 1
        return opkt

    def mkackpkt():
        opkt = SruPacket()
        opkt.ack = 1
        return opkt

    def mkdatapkt(seq, data):
        opkt = SruPacket()
        opkt.seq = seq
        opkt.data = data
        
        return opkt

class EmuTcp(QObject):
    connected = pyqtSignal()
    disconnected = pyqtSignal()
    error = pyqtSignal(int)
    readyRead = pyqtSignal()
    bytesWritten = pyqtSignal()
    newConnection = pyqtSignal()
    
    def __init__(self, parent = None):
        super(EmuTcp, self).__init__(parent)
        self.selfisn = 0
        self.peerisn = 0
        self.selfseq = 0
        self.peerseq = 0

        self.state = 'INIT'
        self.rcvpkts = {}  # 接收缓冲区
        self.rcvwins = {}  # 确认窗口，外部可读取的
        self.sndpkts = {}  # 发送缓冲区
        self.sndwins = {}  # 发送窗口
        self.mode = 'SERVER'  # SERVER|CLIENT

        self.pendCons = []    # pending connectoins
        return

    def mkcon(self):
        self.mode = 'CLIENT'
        self.state = 'SYN_SENT'
        opkt = SruPacket.mkhs1pkt()
        self.selfisn = opkt.seq
        self.selfseq = opkt.seq + 2
        jspkt = opkt.encode()
        return jspkt
    
    def buf_recv_pkt(self, jspkt):
        opkt = SruPacket.decode(jspkt)
        self._statemachine(opkt)

        res = None
        if self.mode == 'SERVER':
            res = self._statemachine_server(opkt)
        elif self.mode == 'CLIENT':
            res = self._statemachine_client(opkt)
        else:
            qDebug('srudp mode error: %s' % self.mode)
        return res

    def buf_send_pkt(self):
        return
    
    def _statemachine_client(self, opkt):
        if self.state == 'SYN_SENT':
            if opkt.syn == 1 and opkt.ack == (self.selfisn + 1) and opkt.seq > 0:
                self.peerisn = opkt.seq
                self.state = 'ESTAB'
                ropkt = SruPacket.mkhs3pkt(self.selfisn, self.peerisn)
                return ropkt.encode()
            else: qDebug('proto error, except SYN_ACK pkt')
        elif self.state == 'ESTAB':
            self.connected.emit()
            pass
        elif self.state == 'FIN1':
            pass
        elif self.state == 'FIN2':
            pass
        elif self.state == 'TIME_WAIT':
            pass
        elif self.state == 'CLOSED':
            pass
        else: qDebug('proto error: %s ' % self.state)
        
        return
    def _statemachine_server(self, opkt):
        if self.state == 'INIT':
            if opkt.syn == 1 and opkt.seq > 0:
                self.state = 'SYN_RCVD'
                self.peerisn = opkt.seq
                self.peerseq = opkt.seq + 2
                ropkt = SruPacket.mkhs2pkt(opkt.seq)
                self.selfisn = ropkt.seq
                self.selfseq = ropkt.seq + 2
                return ropkt.encode()
            else: qDebug('proto error: except syn=1')
        elif self.state == 'SYN_RCVD':
            if opkt.syn == 0 and opkt.ack == (self.selfisn + 1) and opkt.seq == (self.peerisn + 1):
                self.state = 'ESTAB'
                qDebug('server peer ESTABed')
                self.newConnection.emit()
                pass
            else: qDebug('proto error: except syn ack')
        elif self.state == 'ESTAB':
            pass
        elif self.state == 'CLOSE_WAIT':
            pass
        elif self.state == 'LAST_ACK':
            pass
        elif self.state == 'CLOSED':
            pass
        else: qDebug('unkown state: %s' % self.state)
        return


def main():
    #pkt = SruPacket()
    #jspkt = pkt.encode()
    #print(jspkt)
    #opkt = SruPacket.decode(jspkt)
    #print(opkt)
    return


if __name__ == '__main__': main()


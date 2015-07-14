import os, time
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


################
class SrudTransport():
    def __init__(self):
        self.support_binary = False  # 是否支持二进制流
        self.closed = False     # 这一层数据是否传输完成，从逻辑上分析。
        self.can_write = True  # 比较掉线
        return

    # @param data string
    # @return True | False
    def send(self, data): return False

    # @param data bytes | QByteArray
    def encodeData(self, data):  return data

    # @param data bytes | QByteArray
    def encodeHex(self, data):
        if type(data) == str: data = data.encode('utf8')
        if type(data) == bytes: data = QByteArray(data)
        data = data.toHex().data().decode('utf8')
        return data
    def encodeBase64(self, data):
        if type(data) == str: data = data.encode('utf8')
        if type(data) == bytes: data = QByteArray(data)
        encdata = data.toBase64().data().decode('utf8')
        return encdata

    def decodeData(self, data): return data
    def decodeHex(self, data):
        data = QByteArray.fromHex(QByteArray(data)).data()
        return data
    def decodeBase64(self, data):
        decdata = QByteArray.fromBase64(data)
        return decdata 
    
class SrudControl():
    def __init__(self):
        return

# TODO 移动到transport.py
class ToxTunTransport(SrudTransport):
    def __init__(self, tk, peer):
        super(ToxTunTransport, self).__init__()
        self.toxkit = None  # QToxKit instance
        self.peer = ''  # peerid, friend id

        self.toxkit = tk
        self.peer = peer

        self.can_write = True  # 比较掉线
        return

    def send(self, data):
        # try except
        self.toxkit.sendMessage(self.peer, data)
        return False

    def encodeData(self, data): return self.encodeBase64(data)
    def decodeData(self, data): return self.decodeBase64(data)

##################
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
        qDebug(str(jspkt))
        pkt = json.JSONDecoder().decode(jspkt)
        
        opkt = SruPacket2()
        opkt.msg_type = pkt['msgtype']
        opkt.seq = pkt['seq']
        opkt.ack = pkt['ack']
        opkt.data = pkt['data']
        opkt.extra = pkt['extra']

        return opkt
    

class RUDPSendSegment():
    def __init__(self, pkt):
        self.seq = 0
        self.push_ts = QDateTime.currentDateTime()
        self.last_sent_ts = None  # QDateTime
        self.send_count = 0
        self.data = ''

        self.pkt = pkt
        return

    def refreshSentTime(self):
        self.last_sent_ts = QDateTime.currentDateTime()
        self.send_count += 1
        return

    def needResent(self, duration = 567):
        lts = self.last_sent_ts
        if lts is None: lts = self.push_ts

        cts = QDateTime.currentDateTime()
        if lts.msecsTo(cts) >= duration: return True
        
        return False


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
    connectTimeout = pyqtSignal()

    lossPacket = pyqtSignal()
    canClose = pyqtSignal()
    peerClosed = pyqtSignal()
    timeWaitTimeout = pyqtSignal()
    

    def __init__(self, parent = None):
        super(Srudp, self).__init__(parent);
        self.selfisn = 0
        self.peerisn = 0
        self.selfseq = 0
        self.peerseq = 0

        self.chano = 0   # for debug only
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

        self.transport = SrudTransport()  # SrudTrasnport 子类实例
        self.transport = None
        self.step_send_timer = QTimer()
        self.step_send_timer.setInterval(278)
        self.step_send_timer.timeout.connect(self._try_step_send, Qt.QueuedConnection)
        
        self.disconnection_monitor = QTimer()
        self.disconnection_monitor.setInterval(1234)
        self.disconnection_monitor.timeout.connect(self._promise_can_close, Qt.QueuedConnection)
        self.peer_closed = False
        self.self_passive_close = False   # 当前一方是主动关闭还是被动关闭
        self.begin_close_time = None   # 主动关闭发起端发起关闭时间，处理进入TIME_WAIT状态的超时
        
        self.losspkt_monitor = QTimer()
        self.losspkt_monitor.setInterval(678)
        self.losspkt_monitor.timeout.connect(self._on_check_losspkt_timeout, Qt.QueuedConnection)

        self.connect_begin_time = None  # QDateTime
        self.connect_timer = QTimer()
        self.connect_timer.setInterval(1000 * 30)
        self.connect_timer.setSingleShot(True)
        self.connect_timer.timeout.connect(self._on_connect_timeout)
        return

    def setTransport(self, transport):
        self.transport = transport
        return

    # @return True|False
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

        qDebug('aaaaaaaaa')
        qDebug(str(self.transport))
        res = self.transport.send(jspkt)
        self.connect_begin_time = QDateTime.currentDateTime()
        self.connect_timer.start()
        return res

    # @return True|False
    def mkdiscon(self, extra):
        opkt = SruPacket2()
        opkt.msg_type = 'FIN1'
        opkt.extra = extra

        # 在这要判断是主动关闭还是被动关闭
        # 如果还没有接收到peer的FIN1包，则认为本方是主动关闭的。
        if self.self_passive_close is False:
            self.state = 'FIN_WAIT_1'
        else:
            self.state = 'LAST_ACK'

        if self.mode == 'CLIENT':
            opkt.msg_type = 'CLIFIN1'
        else:
            opkt.msg_type = 'SRVFIN1'


        #if self.mode == 'CLIENT':
        #    self.state = 'FIN_WAIT_1'
        #    opkt.msg_type = 'CLIFIN1'
        #else:
        #    self.state = 'LAST_ACK'
        #    opkt.msg_type = 'SRVFIN1'

        
        ### 设置开始关闭时间
        self.begin_close_time = QDateTime.currentDateTime()

        jspkt = opkt.encode()
        res = self.transport.send(jspkt)
        return res


    # @return True | False
    def buf_send_pkt(self, data, extra):
        opkt = self.make_data_pkt(data, extra)
        
        jspkt = opkt.encode()
        res = self.transport.send(jspkt)
        return res

    def make_data_pkt(self, data, extra):
        opkt = SruPacket2()
        opkt.msg_type = 'DATA'
        opkt.seq = self.selfseq
        self.selfseq += 1
        opkt.data = self.transport.encodeData(data)
        opkt.extra = extra
        return opkt

    
    # @return True | False
    def buf_recv_pkt(self, jspkt, extra = None):
        opkt = SruPacket2.decode(jspkt)
        if (opkt.extra == '' or opkt.extra is None) and extra is not None:
            qDebug('warning, rewrite extra value: %s' % str(extra))
            opkt.extra = extra   # for srv SYN2
        elif type(opkt.extra) == dict and extra is not None:
            qDebug('warning, rewrite extra value2: %s' % str(extra))
            # opkt.extra += extra
            for key in extra:
                if key in opkt.extra: qDebug('warning maybe override orignal data')
                opkt.extra[key] = extra[key]
        else: pass
        
        qDebug(str(extra) + ',' + self.state)
        qDebug(str(opkt.extra))
        res = None
        if self.mode == 'SERVER':
            res = self._statemachine_server(opkt)
        elif self.mode == 'CLIENT':
            res = self._statemachine_client(opkt)
        else:
            qDebug('srudp mode error: %s' % self.mode)
        return res


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

                res = self.transport.send(ropkt.encode())
                qDebug('client peer ESTABed')
                self.connect_timer.stop()
                self.connected.emit()
                return res
                # return ropkt.encode()
            else: qDebug('proto error, except SYN_ACK pkt')
        elif self.state == 'ESTAB':
            if opkt.msg_type == 'SRVFIN1':  # 服务器端行发送关闭请求
                self.self_passive_close = True  # 当前一方是被动关闭
                
                ropkt = SruPacket2()
                ropkt.msg_type = 'SRVFIN2'
                ropkt.extra = opkt.extra
                res = self.transport.send(ropkt.encode())
                
                self.state = 'CLOSE_WAIT'
                self.peer_closed = True
                
                return res
                # return ropkt.encode()
            elif opkt.msg_type == 'DATA':
                qDebug('got data: %s' % opkt.encode())
                ropkt = SruPacket2()
                ropkt.msg_type = 'DATA_ACK'
                ropkt.seq = opkt.seq
                ropkt.ack = opkt.seq+1
                ropkt.extra = opkt.extra
                self.rcvpkts[opkt.seq] = opkt
                if opkt.seq > self.maxrcvseq: self.maxrcvseq = opkt.seq

                res = self.transport.send(ropkt.encode())
                QTimer.singleShot(1, self._on_recv_data)
                return res
                # return ropkt.encode()
            elif opkt.msg_type == 'DATA_ACK':
                qDebug('acked data.')
                # 可能有重发的包，需要检测一下，防止报错
                if opkt.seq in self.sndwins: tpkt = self.sndwins.pop(opkt.seq)
                self.attemp_flush()
                # self.bytesWritten.emit()
                self._promise_bytes_written()
            else: qDebug('unexcepted msg type: %s' % opkt.msg_type)
            pass

        ### 主动关闭状态处理
        elif self.state == 'FIN_WAIT_1': self._statemachine_client_close_handler(opkt)
        elif self.state == 'FIN_WAIT_2': self._statemachine_client_close_handler(opkt)
        elif self.state == 'TIME_WAIT': self._statemachine_client_close_handler(opkt)

        ### 被动关闭端状态
        elif self.state == 'CLOSE_WAIT': self._statemachine_client_close_handler(opkt)
        elif self.state == 'LAST_ACK': self._statemachine_client_close_handler(opkt)

        ### 主动与被关闭都存在的状态，但被动端首先进入关闭状态
        elif self.state == 'CLOSED':  self._statemachine_client_close_handler(opkt)
        else: qDebug('proto error: %s ' % self.state)
        
        return


    # 关闭状态机，两端共用，
    def _statemachine_client_close_handler(self, opkt):
        ### 主动关闭状态处理
        if self.state == 'FIN_WAIT_1':
            if opkt.msg_type == 'CLIFIN2':
                self.state = 'FIN_WAIT_2'
                qDebug('emit disconnect event.')
                self.disconnected.emit()
            elif opkt.msg_type == 'SRVFIN2':
                return self._statemachine_client_response_srvfin1(opkt)
            else: qDebug('here')
            pass
        elif self.state == 'FIN_WAIT_2':
            if opkt.msg_type == 'SRVFIN1':
                self.state = 'TIME_WAIT'
                return self._statemachine_client_response_srvfin1(opkt)
            else: qDebug('here')
            pass
        elif self.state == 'TIME_WAIT':
            qDebug('here')
            qDebug('what pkt: %s???' % opkt.msg_type)
            pass

        ### 被动关闭端状态
        elif self.state == 'CLOSE_WAIT':
            qDebug('omited')
            if opkt.msg_type == 'SRVFIN1': qDebug('maybe dup SRVFIN1 pkt')
            pass
        elif self.state == 'LAST_ACK':
            if opkt.msg_type == 'CLIFIN2':
                self.state = 'CLOSED'
                qDebug('cli peer closed.')
                qDebug('emit disconnect event.')
                self.disconnected.emit()
            else: qDebug('here')                
            pass

        ### 主动与被关闭都存在的状态，但被动端首先进入关闭状态
        elif self.state == 'CLOSED':
            qDebug('here')
            if opkt.msg_type == 'SRVFIN1':
                self._statemachine_client_response_srvfin1(opkt)
            else: qDebug('here')
            pass
        else: qDebug('proto error: %s ' % self.state)
        
        return

    def _statemachine_client_response_srvfin1(self, opkt):
        ropkt = SruPacket2()
        ropkt.msg_type = 'SRVFIN2'
        ropkt.extra = opkt.extra

        res = self.transport.send(ropkt.encode())
        time.sleep(0.0001)
        res = self.transport.send(ropkt.encode())
        self.peer_closed = True
        self.peerClosed.emit()
        qDebug('emit disconnect event.')
        self.disconnected.emit()

        return res

    def _statemachine_server_response_clifin1(self, opkt):
        ropkt = SruPacket2()
        ropkt.msg_type = 'CLIFIN2'
        ropkt.extra = opkt.extra

        res = self.transport.send(ropkt.encode())
        time.sleep(0.0001)
        res = self.transport.send(ropkt.encode())
        self.peer_closed = True
        self.peerClosed.emit()
        qDebug('emit disconnect event.')
        self.disconnected.emit()
        return res


    # 关闭状态机，两端共用，
    def _statemachine_server_close_handler(self, opkt):
        ### 主动关闭状态处理
        if self.state == 'FIN_WAIT_1':
            if opkt.msg_type == 'SRVFIN2':
                self.state = 'FIN_WAIT_2'
                # if self.peer_closed:
                qDebug('emit disconnect event.')
                self.disconnected.emit()
            elif opkt.msg_type == 'CLIFIN1':
                self.state = 'TIME_WAIT'
                return self._statemachine_server_response_clifin1(opkt)
            else: qDebug('here')
            pass
        elif self.state == 'FIN_WAIT_2':
            if opkt.msg_type == 'CLIFIN1':
                self.state = 'TIME_WAIT'
                return self._statemachine_server_response_clifin1(opkt)
            elif opkt.msg_type == 'SRVFIN2':
                self.state = 'TIME_WAIT'
                qDebug('emit disconnect event.')
                self.disconnected.emit()
            else: qDebug('here')
            pass
        elif self.state == 'TIME_WAIT':
            qDebug('here')
            qDebug('what pkt: %s???' % opkt.msg_type)
            pass

        ### 被动关闭端状态
        elif self.state == 'CLOSE_WAIT':
            qDebug('omited')
            if opkt.msg_type == 'CLIFIN1': qDebug('maybe dup CLIFIN1 pkt')
            pass
        elif self.state == 'LAST_ACK':
            if opkt.msg_type == 'SRVFIN2':
                self.state = 'CLOSED'
                qDebug('srv peer closed.')
                qDebug('emit disconnect event.')
                self.disconnected.emit()
            else: qDebug('here')
            pass

        ### 主动与被关闭都存在的状态，但被动端首先进入关闭状态
        elif self.state == 'CLOSED':
            qDebug('here')
            if opkt.msg_type == 'CLIFIN1':
                res = self._statemachine_server_response_clifin1(opkt)
            else: qDebug('here')
            pass
        else: qDebug('proto error: %s ' % self.state)
        
        return

    
    # @return True|False|None
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

                res = self.transport.send(ropkt.encode())
                return res
                # return ropkt.encode()
            else: qDebug('proto error: except syn=1')
        elif self.state == 'SYN_RCVD':
            if opkt.msg_type == 'SYN_ACK' and opkt.ack == (self.selfisn + 1) and opkt.seq == (self.peerisn + 1):
                self.state = 'ESTAB'
                qDebug('server peer ESTABed')
                self.newConnection.emit()
                self.step_send_timer.start()
                self.losspkt_monitor.start()
                pass
            else: qDebug('proto error: except syn ack')
        elif self.state == 'ESTAB':
            if opkt.msg_type == 'CLIFIN1':
                self.self_passive_close = True
                
                ropkt = SruPacket2()
                ropkt.msg_type = 'CLIFIN2'
                ropkt.extra = opkt.extra
                res = self.transport.send(ropkt.encode())
                self.state = 'CLOSE_WAIT'
                
                return res
                # return ropkt.encode()
            elif opkt.msg_type == 'DATA':
                qDebug('got data. %s' % opkt.encode())
                ropkt = SruPacket2()
                ropkt.msg_type = 'DATA_ACK'
                ropkt.seq = opkt.seq
                ropkt.ack = opkt.seq+1
                ropkt.extra = opkt.extra
                self.rcvpkts[opkt.seq] = opkt
                if opkt.seq > self.maxrcvseq: self.maxrcvseq = opkt.seq

                res = self.transport.send(ropkt.encode())
                QTimer.singleShot(1, self._on_recv_data)
                return res
                # return ropkt.encode()
            elif opkt.msg_type == 'DATA_ACK':
                qDebug('acked data.')
                # 可能有重发的包，需要检测一下，防止报错
                if opkt.seq in self.sndwins: tpkt = self.sndwins.pop(opkt.seq)
                else: qDebug('maybe resent pkg\'s ack: %d' % opkt.seq)
                self.attemp_flush()
                # self.bytesWritten.emit()
                self._promise_bytes_written()
            else: qDebug('unexcepted msg type: %s' % opkt.msg_type)
            pass

        ### 主动关闭状态处理
        elif self.state == 'FIN_WAIT_1': self._statemachine_server_close_handler(opkt)
        elif self.state == 'FIN_WAIT_2': self._statemachine_server_close_handler(opkt)
        elif self.state == 'TIME_WAIT': self._statemachine_server_close_handler(opkt)

        ### 被动关闭端状态
        elif self.state == 'CLOSE_WAIT': self._statemachine_server_close_handler(opkt)
        elif self.state == 'LAST_ACK': self._statemachine_server_close_handler(opkt)

        ### 主动与被关闭都存在的状态，但被动端首先进入关闭状态
        elif self.state == 'CLOSED':  self._statemachine_server_close_handler(opkt)
        else: qDebug('proto error: %s ' % self.state)

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


    # 好像只能使用buf 28,再多容易导致tox掉线
    CCC_BUF_SIZE = 10     # 也代表了每批次发送的包数量，太大的话toxnet容易掉线，并且对不同的网络值还不一样。
    CCC_WIN_SIZE = 208     # 最大发送窗口大小 
    # CCC_PKT_CNT_PER_TIME = 20     # 每次发送包个数

    # 发送窗口相关控制，包重发控制
    def attemp_send(self, data, extra):
        ccc_buf_size = Srudp.CCC_BUF_SIZE
        ccc_win_size = Srudp.CCC_WIN_SIZE

        res = False
        if len(self.sndpkts) < ccc_buf_size:
            res = True
            opkt = self.make_data_pkt(data, extra)
            self.sndpkts[opkt.seq] = opkt
            pass

        if len(self.sndpkts) >= (ccc_buf_size/3) or len(self.sndwins) < ccc_buf_size: self.attemp_flush()
        return res

    def attemp_flush(self):
        ccc_buf_size = Srudp.CCC_BUF_SIZE
        ccc_win_size = Srudp.CCC_WIN_SIZE

        if len(self.sndwins) >= ccc_win_size: return 0
        
        dkeys = self.sndpkts.keys()
        keys = []
        for key in dkeys: keys.append(key)
        wrcnt = 0
        while len(keys) > 0 and len(self.sndwins) < ccc_win_size:
            key = min(keys)
            opkt = self.sndpkts[key]
            spkt = RUDPSendSegment(opkt)
            # self.sndwins[key] = opkt
            self.sndwins[key] = spkt
            self.sndpkts.pop(key)
            keys.remove(key)
            # keys.pop(key)
            wrcnt += 1
            self.transport.send(spkt.pkt.encode())
        qDebug('send net count: %d, win: %d, buf: %d @%d-%s' %
               (wrcnt, len(self.sndwins), len(self.sndpkts), self.chano, self.state))
        return wrcnt
    
    def getLossPackets(self):
        return self.rcvlosspkts

    def readPacket(self):
        if len(self.rcvwins) > 0:
            mkey = min(self.rcvwins.keys())
            opkt = self.rcvwins.pop(mkey)
            return opkt
        return None

    def _try_step_send(self):
        # qDebug('here')

        n = 0
        n = self.attemp_flush()
        if n > 0:
            qDebug('step send: %d' % n)
            # self.bytesWritten.emit()
            self._promise_bytes_written()

        qDebug('blen: %d, wlen: %d @%d-%s' % (len(self.sndpkts), len(self.sndwins), self.chano, self.state))
            
        return

    def _promise_bytes_written(self):
        if len(self.sndpkts) < self.CCC_BUF_SIZE:
            self.bytesWritten.emit()
        return

    def startCheckClose(self):
        self.disconnection_monitor.start()
        return

    def _promise_can_close(self):
        # qDebug('here')
        # 确认传输层已经关闭
        if self.transport.closed is False:
            qDebug('transport layer is not closed')
            pass

        # 确认状态
        # self.state
        if self.state == 'TIME_WAIT':
            nowtime = QDateTime.currentDateTime()
            duration = self.begin_close_time.msecsTo(nowtime)
            if duration > 15000:  # 15 secs
                # 触发TIME_WAIT timeout事件: timeWaitTimeout
                self.step_send_timer.stop()
                self.losspkt_monitor.stop()
                self.disconnection_monitor.stop()
                self.timeWaitTimeout.emit()
                pass
            return
        elif self.state == 'CLOSED':
            return

        # 查看是否还有需要外发的包。
        bplen = len(self.sndpkts)
        wplen = len(self.sndwins)

        can_close = False
        if bplen == 0 and wplen == 0:
            can_close = True
            pass
        else:
            qDebug('wait a moment, bplen=%d, wplen=%d' % (bplen, wplen))
            pass

        if can_close is True:
            # 停止检测计时器
            # self.disconnection_monitor.stop()
            # 发送关闭FIN1包。
            qDebug('can close  now')
            self.canClose.emit()

            # 进入慢轮循TIME_WAIT状态
            if self.self_passive_close is False:
                self.disconnection_monitor.setInterval(1000*3)
                self.disconnection_monitor.stop()
                self.disconnection_monitor.start()
            else: self.disconnection_monitor.stop()
            pass
        
        return


    def _on_check_losspkt_timeout(self):
        qDebug('here')
        ack_timeout = 1234
        
        resent_keys = []
        for sk in self.sndwins:
            spkt = self.sndwins[sk]
            if spkt.needResent(ack_timeout):
                resent_keys.append(sk)

        #
        rscnter = Srudp.CCC_BUF_SIZE
        for sk in resent_keys:
            if rscnter <= 0: break
            spkt = self.sndwins[sk]
            spkt.refreshSentTime()
            self.transport.send(spkt.pkt.encode())
            rscnter -= 1
            pass
        
        qDebug('rs: %d/%d' % (Srudp.CCC_BUF_SIZE - rscnter, len(resent_keys)))    
        return

    def _on_connect_timeout(self):
        qDebug('here')
        if self.state == 'SYN_SENT':
            qDebug('connect timeout')
            self.connectTimeout.emit()
            pass
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


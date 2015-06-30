
from PyQt5.QtCore import *

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


class Srudp(QObject):
    def __init__(self, parent = None):
        super(Srudp, self).__init__(parent)
        self.selfisn = 0
        self.peerisn = 0
        self.state = 'INIT'
        self.rdpkts = {}
        self.wrpkts = {}
        self.mode = 'SERVER'  # SERVER|CLIENT
        return

    def mkcon(self):
        self.mode = 'CLIENT'
        self.state = 'SYN_SENT'
        opkt = SruPacket.mkhs1pkt()
        jspkt = opkt.encode()
        return jspkt
    
    def recved(self, jspkt):
        opkt = SruPacket.decode(jspkt)
        self._statemachine(opkt)

        if self.mode == 'SERVER':
            self._statemachine_server(opkt)
        elif self.mode == 'CLIENT':
            self._statemachine_client(opkt)
        else:
            qDebug('srudp mode error: %s' % self.mode)
        return

    def _statemachine_client(self, opkt):
        
        return
    def _statemachine_server(self, opkt):
        if self.state == 'INIT':
            if opkt.syn == 1 and opkt.seq > 0:
                self.state = 'SYN_RCVD'
                self.peerisn = opkt.seq
                self.selfisn = SruPacket.randISN()
                ropkt = SruPacket.mkhs2pkt(opkt.seq)
                return ropkt.encode()
            else: qDebug('proto error: except syn=1')
        elif self.state == 'SYN_RCVD':
            if opkt.syn == 0 and opkt.ack == (self.selfisn + 1) and opkt.seq == (self.peerisn + 1):
                self.state = 'ESTAB'
                pass
            else: qDebug('proto error: except syn ack')
        elif self.state == 'ESTAB':
            
        else: qDebug('unkown state: %s' % self.state)
        return


def main():
    pkt = SruPacket()
    jspkt = pkt.encode()
    print(jspkt)
    opkt = SruPacket.decode(jspkt)
    print(opkt)
    return


if __name__ == '__main__': main()


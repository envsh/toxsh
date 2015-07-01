
from srudp import *

def test_main():
    import json

    extra = {}
    cliudp = Srudp()
    cli_synpkt = cliudp.mkcon(extra)
    print('cli SYN:', cli_synpkt)
    
    srvudp = Srudp()
    syn2pkt = srvudp.buf_recv_pkt(cli_synpkt)
    print(srvudp, srvudp.state, srvudp.mode)
    print('srv SYN2:', syn2pkt)

    synackpkt = cliudp.buf_recv_pkt(syn2pkt)
    print('cli SYN_ACK:', synackpkt)

    srvudp.buf_recv_pkt(synackpkt)
    # server connected


    d1pkt = SruPacket2()
    d1pkt.msg_type = 'DATA'
    d1pkt.data = 'joiafjiewafjewafawehoieafwfhu from cli'
    d1pkt = d1pkt.encode()
    srvudp.buf_recv_pkt(d1pkt)

    d1pkt = SruPacket2.decode(d1pkt)
    d1pkt.data = 'joiafjiewafjewafawehoieafwfhu from srv'
    d1pkt = d1pkt.encode()
    cliudp.buf_recv_pkt(d1pkt)

    fin1pkt = cliudp.mkdiscon(extra)
    print('cli FIN1', fin1pkt)

    fin2pkt = srvudp.buf_recv_pkt(fin1pkt)
    print('srv FIN2', fin2pkt)

    cliudp.buf_recv_pkt(fin2pkt)
    print(cliudp.state)

    srvfin1pkt = srvudp.mkdiscon(extra)
    print('srv FIN1', srvfin1pkt, srvudp.state)

    clifin2pkt = cliudp.buf_recv_pkt(srvfin1pkt)
    print('cli FIN2:', clifin2pkt, cliudp.state)

    srvudp.buf_recv_pkt(clifin2pkt)
    print(srvudp.state)
    

if __name__ == '__main__': test_main()




_srvpeer = '6BA28AC06C1D57783FE017FA9322D0B356E61404C92155A04F64F3B19C75633E8BDDEFFA4856'

class ToxTunRecord():
    def __init__(self):
        self.local_host = ''
        self.local_port = 0
        self.remote_host = ''
        self.remote_port = 0
        self.remote_pubkey = ''
        return
    
class ToxTunConfig():
    def __init__(self, config_file):
        self.config_file = config_file
        self.recs = []

        self.load()
        return

    def load(self):
        rec = ToxTunRecord()
        rec.local_host = '*'
        rec.local_port = 8080
        rec.remote_host = '127.0.0.1'
        rec.remote_port = 80
        rec.remote_pubkey = _srvpeer
        self.recs.append(rec)

        rec = ToxTunRecord()
        rec.local_host = '*'
        rec.local_port = 8181
        rec.remote_host = '127.0.0.1'
        rec.remote_port = 81
        rec.remote_pubkey = _srvpeer
        #self.recs.append(rec)

        rec = ToxTunRecord()
        rec.local_host = '*'
        rec.local_port = 8282
        rec.remote_host = '127.0.0.1'
        rec.remote_port = 82
        rec.remote_pubkey = _srvpeer
        #self.recs.append(rec)
        
        return

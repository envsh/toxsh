###
## 通用http服务，使用python重写的qhttpserver
## TODO 内存漏洞问题太严重，为什么呢？
## 应该是python有一个最大使用内存上限的问题，内存用上去就很难下来
## 如果请求量均匀，则不会使用太多内存，python总会重用之前请求释放的内存，但不释放给系统。
## 不过还有可能，内存占用量会越来越小。
## TODO 简单http服务统计，需要吗？

import sys
import time
import enum


try:
    from http_parser.parser import HttpParser
except:
    from http_parser.pyparser import HttpParser

from PyQt5.QtCore import *
from PyQt5.QtNetwork import *

class HttpMethod(enum.IntEnum):
    DELETE = 1,
    GET = 2,
    HEAD = 3,
    POST = 4,
    PUT = 5,
    #// pathological
    CONNECT = 6,
    OPTIONS = 7,
    TRACE = 8,
    #// webdav
    COPY = 9,
    LOCK = 10,
    MKCOL = 11,
    MOVE = 12,
    PROPFIND = 13,
    PROPPATCH = 14,
    SEARCH = 15,
    UNLOCK = 16,
    #// subversion
    REPORT = 17,
    MKACTIVITY = 18,
    CHECKOUT = 19,
    MERGE = 20,
    #// upnp
    MSEARCH = 21,
    NOTIFY = 22,
    SUBSCRIBE = 23,
    UNSUBSCRIBE = 24,
    #// RFC-5789
    PATCH = 25,
    PURGE = 26
   
class HTTP_STATUS(enum.IntEnum):
    CONTINUE = 100,
    SWITCH_PROTOCOLS = 101,
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NON_AUTHORITATIVE_INFORMATION = 203,
    NO_CONTENT = 204,
    RESET_CONTENT = 205,
    PARTIAL_CONTENT = 206,
    MULTIPLE_CHOICES = 300,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    SEE_OTHER = 303,
    NOT_MODIFIED = 304,
    USE_PROXY = 305,
    TEMPORARY_REDIRECT = 307,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    PAYMENT_REQUIRED = 402,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    NOT_ACCEPTABLE = 406,
    PROXY_AUTHENTICATION_REQUIRED = 407,
    REQUEST_TIMEOUT = 408,
    CONFLICT = 409,
    GONE = 410,
    LENGTH_REQUIRED = 411,
    PRECONDITION_FAILED = 412,
    REQUEST_ENTITY_TOO_LARGE = 413,
    REQUEST_URI_TOO_LONG = 414,
    REQUEST_UNSUPPORTED_MEDIA_TYPE = 415,
    REQUESTED_RANGE_NOT_SATISFIABLE = 416,
    EXPECTATION_FAILED = 417,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505

# simple usage:
# print(HTTP_STATUS.GONE, int(HTTP_STATUS.GONE), HTTP_STATUS.GONE == 410)

#######
_STATUS_CODES = {
    100: "Continue",    
    101: "Switching Protocols",    
    102: "Processing",     # RFC 2518) obsoleted by RFC 4918
    200: "OK",    
    201: "Created",    
    202: "Accepted",    
    203: "Non-Authoritative Information",    
    204: "No Content",    
    205: "Reset Content",    
    206: "Partial Content",    
    207: "Multi-Status",     # RFC 4918
    300: "Multiple Choices",    
    301: "Moved Permanently",    
    302: "Moved Temporarily",    
    303: "See Other",    
    304: "Not Modified",    
    305: "Use Proxy",    
    307: "Temporary Redirect",    
    400: "Bad Request",    
    401: "Unauthorized",    
    402: "Payment Required",    
    403: "Forbidden",    
    404: "Not Found",    
    405: "Method Not Allowed",    
    406: "Not Acceptable",    
    407: "Proxy Authentication Required",    
    408: "Request Time-out",    
    409: "Conflict",    
    410: "Gone",    
    411: "Length Required",    
    412: "Precondition Failed",    
    413: "Request Entity Too Large",    
    414: "Request-URI Too Large",    
    415: "Unsupported Media Type",    
    416: "Requested Range Not Satisfiable",    
    417: "Expectation Failed",    
    418: "I\"m a teapot",            # RFC 2324
    422: "Unprocessable Entity",     # RFC 4918
    423: "Locked",                   # RFC 4918
    424: "Failed Dependency",        # RFC 4918
    425: "Unordered Collection",     # RFC 4918
    426: "Upgrade Required",         # RFC 2817
    500: "Internal Server Error",    
    501: "Not Implemented",    
    502: "Bad Gateway",    
    503: "Service Unavailable",    
    504: "Gateway Time-out",    
    505: "HTTP Version not supported",    
    506: "Variant Also Negotiates",     # RFC 2295
    507: "Insufficient Storage",        # RFC 4918
    509: "Bandwidth Limit Exceeded",    
    510: "Not Extended",     # RFC 2774    
}

### 会回收的。
import gc
# gc.set_debug(gc.DEBUG_LEAK)
# print(gc.isenabled())
# gc.disable()
# print(gc.isenabled())

_qogc = None
class _QObjectGC(QObject):
    def __init__(self):
        super(_QObjectGC, self).__init__()

        self.m_qobjs = []
        self.m_last_count = 0

        self._MAX_TIMEOUT = 1000
        self.m_timer = QTimer()
        #self.m_timer = None
        self.m_timer.timeout.connect(self._onTimeout)
        self.m_timer.setSingleShot(True)
        return

    def add(self, qobj):
        qobj.destroyed.connect(self._onQObjectDestroyed)
        self.m_qobjs.append(qobj)

        cnt = len(self.m_qobjs)
        # if cnt > 100000: qWarning('too much qt object: %d' % cnt)
        # qDebug(str(gc.is_tracked(qobj)))
        return

    ### temporary
    def run(self):
        self.m_timer = QTimer()
        self.m_timer.timeout.connect(self._onTimeout)
        self.m_timer.setSingleShot(True)
        self.m_timer.start(self._MAX_TIMEOUT)
        self.exec_()
        return
    
    def start(self):
        self.m_timer.start(self._MAX_TIMEOUT)
        return
    
    def _onTimeout(self):
        cnt = len(self.m_qobjs)
        if cnt != self.m_last_count:
            qDebug('now qoc: ' + str(cnt))
        self.m_last_count = cnt
        
        btime = time.time()

        toomuch = False
        gced = 0
        tobjs = []
        for qobj in self.m_qobjs:
            refcnt = sys.getrefcount(qobj)
            # qDebug(str(qobj) + ',' + str(refcnt))
            if refcnt == 3:
                #qDebug('need gc obj:' + str(qobj) + ',' + qobj.objectName())
                tobjs.append(qobj)
                self.m_qobjs.remove(qobj)
                gced += 1
                if gced > 1000: toomuch = True; break
                
        if gced > 0: qDebug('gced:%d/len:%d.' %(gced, cnt))

        etime = time.time()
        dtime = etime - btime
        if gced > 0: qDebug("used time for walk all objs: " + str(dtime))

        kdtime = dtime * 1000
        next_timeout = self._MAX_TIMEOUT
        if kdtime >= self._MAX_TIMEOUT or toomuch: next_timeout = 10
        else: next_timeout = self._MAX_TIMEOUT - kdtime
        self.m_timer.start(int(next_timeout))

        ### debug
        #gcsts = gc.get_stats()
        #qDebug('cls=%d, cld=%d, ucl=%d' % (len(gcsts.collections), len(gcsts.collected), len(gcsts.uncollectable)))
        # qDebug(str(len(gc.garbage)))

        ### python will free it here
        if gced > 0:
            btime = time.time()
            tobjs = None
            etime = time.time()
            dtime = etime - btime
            sys.stdout.write('=' + str(gced) + ', gctime: ' + str(dtime) + "\n")
            sys.stdout.flush()
        
        return

    def _onQObjectDestroyed(self, qobj):
        # 当前的这个self并不是_QObjectGC对象本身？？？
        # qDebug(str(self))
        tobj = _qogc.sender()
        in_op = tobj in self.m_qobjs
        equal = tobj == _qogc
        # qDebug('in op: %s, equal: %s, %s, %s' % (in_op, equal, tobj, qobj))
        sys.stdout.write('~')
        return
    
    def last(self): return

### 实例化
_qogc = _QObjectGC()

#######
class QHttpRequest(QObject):
    readyRead = pyqtSignal()
    
    def __init__(self, conn, parent = None):
        super(QHttpRequest, self).__init__(parent)
        # self.m_conn = conn
        self.m_parser = conn.m_parser

    def method(self): return self._methodToEnum(self.m_parser.get_method())
    def methodString(self): return self.m_parser.get_method()
    def url(self): return self.m_parser.get_url()
    def path(self): return self.m_parser.get_path()
    def version(self): return self.m_parser.get_version()
    def queryString(self): return self.m_parser.get_query_string()
    def fragment(self): return self.m_parser.get_fragment()
    def headers(self): return self.m_parser.get_headers()
    def header(self, field): return self.m_parser.get_headers().get(field)
    def isProxyMode(self):
        return 'PROXY-CONNECTION' in self.m_parser.get_headers()
    def hasBody(self): self.readyRead.emit()
    def readBody(self): return self.m_parser.recv_body()
    
    def _methodToEnum(mth):
        for ea in HttpMethod:
            s = str(ea)
            if s[len('HttpMethod.'):] == mth:
                #print('found it:' + str(a))
                #print(int(a))
                return ea
        return
    def _dump(self):
        qDebug(self.methodString())
        qDebug(self.url())
        qDebug(self.path())
        qDebug(str(self.headers()))
        qDebug(str(self.isProxyMode()))
        return

class QHttpResponse(QObject):
    done = pyqtSignal()
    
    def __init__(self, conn, parent = None):
        super(QHttpResponse, self).__init__(parent)
        self.m_conn = conn
        self.m_headers = {}
        self.m_finished = False
        
    def setHeader(self, field, value):
        self.m_headers[field] = value
        return

    def writeHead(self, code):
        if type(code) == HTTP_STATUS: code = int(code)
        
        scline = "HTTP/1.1 %d %s\r\n" % (code, _STATUS_CODES[code])
        self.m_conn.write(scline)

        self._writeHeaders()
        self.m_conn.write("\r\n")
        
        return

    ### 如果是chunked或者Content-Encoding是gzip，需要调用端编译好data数据
    ### 在这个函数中不负责编码这些数据。
    ### 不能用来write header
    def write(self, data):
        qDebug(str(self.m_headers))
        if 'TRANSFER-ENCODING' in self.m_headers:
            self.writeChunked(data)
        else:
            self.writeRaw(data)
        return

    def writeChunked(self, data):
        dlen = len(data)
        hlen = hex(dlen)[2:]
        hlen = hlen.upper()
        qDebug(hlen)
        if dlen == 0:
            self.m_conn.write("0\r\n\r\n")
            self.m_conn.close()
        else:
            self.m_conn.write(hlen + "\r\n")
            self.m_conn.write(data)
            self.m_conn.write("\r\n")
        return

    def writeRaw(self, data):
        if len(data) == 0:
            self.m_conn.close()
        else:
            self.m_conn.write(data)
        
        return
    
    def end(self, data = ''):
        if self.m_finished:
            qWarning("QHttpResponse::end() Cannot write end after response has finished.")
        if len(data) > 0: self.write(data)
        self.m_finished = True
        self.done.emit()
        # cleanup
        self.m_conn = None
    
    def _writeHeader(self, field, value):
        self.m_conn.write(field)
        self.m_conn.write(': ')
        self.m_conn.write(value)
        self.m_conn.write("\r\n")
        return
    
    ### 要能够处理基本的header响应，而不能全部让调用端设置
    def _writeHeaders(self):

        t_last = False
        t_keep_alive = False
        t_send_transfer_encoding = False
        t_use_trunk_encoding = False
        t_send_content_length = False
        t_send_connection = False
        
        has_connection = 'Connection' in self.m_headers
        if has_connection:
            t_send_connection = True
            value = self.m_headers['Connection']
            if value == 'Close': t_last = True
            else: t_keep_alive = True

        has_transfer_encoding = 'Transfer-Encoding' in self.m_headers
        if has_transfer_encoding:
            t_sent_transfer_encoding = True
            value = self.m_headers['Transfer-Encoding']
            if value == 'chunked': t_use_trunk_encoding = True

        if not t_send_connection:
            if t_keep_alive and (t_send_content_length or t_use_trunk_encoding):
                self.m_headers['Connection'] = 'keep-alive'
            else:
                self.m_headers['Connection'] = 'Close'

        if not t_send_content_length and not t_send_transfer_encoding:
            if t_use_trunk_encoding:
                self.m_headers['Transfer-Encoding'] = 'chunked'
            else:
                t_last = True
                self.m_headers['Connection'] = 'Close'
                
        if 'Content-Type' not in self.m_headers:
            self.m_headers['Content-Type'] = 'text/plain; charset=UTF-8'
        if 'Content-Length' not in self.m_headers:
            self.m_headers['Transfer-Encoding'] = 'chunked'
        if 'Server' not in self.m_headers:
            self.m_headers['Server'] = 'qhttpd.py/1.0'
        if 'Date' not in self.m_headers:
            self.m_headers['Date'] = QLocale.c().toString(QDateTime.currentDateTimeUtc(), "ddd, dd MMM yyyy hh:mm:ss") + " GMT"

        # print(self.m_headers)
        for field in self.m_headers:
            # qDebug(field)
            self._writeHeader(field, self.m_headers[field])
            
        return
    
######
class QHttpConnection(QObject):
    newRequest = pyqtSignal(QHttpRequest, QHttpResponse)
    disconnected = pyqtSignal()
    
    def __init__(self, sock, parent = None):
        super(QHttpConnection, self).__init__(parent)

        self.m_sock = sock
        self.m_body = []
        self.m_parser = HttpParser()

        self.m_request = QHttpRequest(self)
        self.m_request = None
        self.m_response = QHttpResponse(self)
        self.m_response = None
        
        self.m_sock.readyRead.connect(self._onReadyRead)
        self.m_sock.disconnected.connect(self._onDisconnected)
        self.m_sock.bytesWritten.connect(self._onBytesWritten)
        
        return
    
    def write(self, data):
        self.m_sock.write(data)
        return

    def _onReadyRead(self):
        #qDebug('hehe')
        qtdata = self.m_sock.readAll()
        pydata = qtdata.data()
        np = self.m_parser.execute(pydata, len(pydata))
        qDebug(str(np) + '=?' + str(len(pydata)))
        #qDebug(qtdata)
        #qDebug(qtdata.toHex())
        #print(self.m_parser._body)
        #print(self.m_parser._body)

        #qDebug(str(self.m_parser.is_message_begin()))
        #qDebug(str(self.m_parser.is_message_complete()))
        #qDebug(str(self.m_parser.is_headers_complete()))

        if self.m_parser.is_headers_complete():
            if self.m_request != None:
                qWarning('alread have a request object')
            else:
                self.m_request = QHttpRequest(self)
                _qogc.add(self.m_request)
                # qDebug(str(self.m_request))
                # print(self.m_parser.get_headers())
            True

        ### body area
        # qDebug(str(self.m_parser.is_message_begin()))
        # not use lines，这个可能指的是在客户端时，数据下载完成标识吧。
        if self.m_parser.is_message_begin() and self.m_request != None:
            qDebug('body coming...')
            self.m_request.hasBody()
            
        mth = self.m_parser.get_method()
        # qDebug(mth)
            
        if mth == 'GET':
            if self.m_parser.is_headers_complete():
                self.m_response = QHttpResponse(self)
                self.m_response.done.connect(self._onResponseDone)
                _qogc.add(self.m_response)

                self.newRequest.emit(self.m_request, self.m_response)
            pass
        elif mth == 'POST':
            if self.m_parser.is_partial_body(): self.m_body.append(self.m_parser.recv_body())
            if self.m_parser.is_message_complete(): print(b''.join(self.m_body))
        elif mth == 'CONNECT':
            if self.m_parser.is_headers_complete():
                if self.m_response != None:
                    qWarning('alread have a response object')
                else:
                    self.m_response = QHttpResponse(self)
                    self.m_response.done.connect(self._onResponseDone)
                    _qogc.add(self.m_response)

                    self.newRequest.emit(self.m_request, self.m_response)
            else:
                qDebug('hdr not complete')
            True
        else:
            qWarning("not impled method:" + mth)
            self.m_sock.close()
        
        return

    def _onDisconnected(self):
        # qDebug('hehe')
        self.disconnected.emit()
        return

    def _onBytesWritten(self, count):
        # qDebug('hehe')
        return

    def _onResponseDone(self):
        self.m_sock.disconnectFromHost()
        self.m_sock.close()
        # qDebug(str(self.m_request))
        return

    def close(self):
        self.m_sock.flush()
        self.m_sock.close()
        return

    def last(self): return
    
######
class QHttpServer(QObject):
    newRequest = pyqtSignal(QHttpRequest, QHttpResponse)
    
    def __init__(self, parent = None):
        super(QHttpServer, self).__init__(parent)

        self.m_serv = QTcpServer()
        self.m_serv = None
        self.m_conns = []
        self.m_connid = 0

        self.m_sesses = {}

        _qogc.start()
        
    def listen(self, port = 8000, host = QHostAddress.Any):
        self.m_serv = QTcpServer(self)

        ok = self.m_serv.listen(QHostAddress.Any, port)
        if ok: self.m_serv.newConnection.connect(self._onNewConnection)
        else: self.m_serv = None
        
        return ok

    def close(self):
        return

    def _onNewConnection(self):
        sock = self.m_serv.nextPendingConnection()
        # qDebug(str(sock.parent()))
        sock.setParent(None)
        self.m_connid += 1
        sock.setObjectName('connsock.' + str(self.m_connid))
        _qogc.add(sock)
        
        conn = QHttpConnection(sock)
        self.m_conns.append(conn)
        _qogc.add(conn)
        conn.disconnected.connect(self._onConnectionDisconnected)
        conn.newRequest.connect(self.newRequest)

        # qDebug('new conn here')
        return
    
    def _onConnectionDisconnected(self):
        conn = self.sender()
        if conn in self.m_conns:
            # qDebug('remove conn: ' + str(conn))
            self.m_conns.remove(conn)
        else:
            qDebug('maybe leaked conn: ' + str(conn))            
        return
    
    def last(self): return
    
def _main():
    class _ServerTester(QObject):
        def __init__(self, parent = None):
            #super(ServerTester, self).__init__(parent)
            QObject.__init__(self)
            print(super())

            return

        def onNewRequest(self, request, response):
            #qDebug("here, " + str(request))
            response.setHeader('Content-Length', '12')
            response.writeHead(200)
            response.write('12345\n12345\n')
            response.end()
        
    import qtutil

    qInstallMessageHandler(qtutil.qt_debug_handler)
    a = QCoreApplication(sys.argv)
    qtutil.pyctrl()
    # _qogc.start()
    
    httpd = QHttpServer()
    httpd.listen()
    # httpd.close()

    tster = _ServerTester()
    httpd.newRequest.connect(tster.onNewRequest)
    print(tster.objectName())
    
    a.exec_()
    
############
if __name__ == '__main__':
    _main()


#####
# http://enki-editor.org/2014/08/23/Pyqt_mem_mgmt.html


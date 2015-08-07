
#include <tox/tox.h>

#include "serializer.h"

QByteArray serialize_packet_dep(const ENetAddress * address, const ENetBuffer * buffer)
{
    QByteArray data;
    
    struct msghdr msgHdr = {0};
    struct sockaddr_in sin = {0};
    int sentLength;

    memset (& msgHdr, 0, sizeof (struct msghdr));
    
    if (address != NULL)
    {
        memset (& sin, 0, sizeof (struct sockaddr_in));
        sin.sin_family = AF_INET;
        sin.sin_port = ENET_HOST_TO_NET_16 (address -> port);
        sin.sin_addr.s_addr = address -> host;

        msgHdr.msg_name = & sin;
        msgHdr.msg_namelen = sizeof (struct sockaddr_in);
    }
    
    data.append((char*)&sin, sizeof(struct sockaddr_in));
    
    data.append((char*)&buffer->dataLength, sizeof(buffer->dataLength));
    data.append((char*)buffer->data, buffer->dataLength);
    // qDebug()<<buffer->dataLength;

    return data;
    
    // msgHdr.msg_iov = (struct iovec *) buffers;
    // msgHdr.msg_iovlen = bufferCount;

    // sentLength = sendmsg (socket, & msgHdr, MSG_NOSIGNAL);
    
    // if (sentLength == -1)
    // {
    //    if (errno == EWOULDBLOCK)
    //      return 0;

    //    return -1;
    // }

    // return sentLength;
}

static bool test_deserialize_packet(const QByteArray pkt)
{
    // deserialize
    struct sockaddr_in sin = {0};
    const char *pktdata = pkt.constData();
    memcpy(&sin, pktdata, sizeof(struct sockaddr_in));
    size_t length = 0;
    memcpy(&length, pktdata + sizeof(struct sockaddr_in), sizeof(size_t));
    int recvLength = 0;
    recvLength = length;
    
    // qDebug()<<pkt.length()<<sizeof(struct sockaddr_in)<<sizeof(size_t)<<length;
    if (pkt.length() != (sizeof(struct sockaddr_in) + sizeof(size_t) + length)) {
        qDebug()<<"warning size not match excepted:"
                <<pkt.length()<<(sizeof(struct sockaddr_in) + sizeof(size_t) + length)
                <<sizeof(struct sockaddr_in)<<sizeof(size_t)<<length;
        return false;
    } else  {
        return true;
    }
    
    return false;
}

QByteArray serialize_packet(const ENetAddress* address, const ENetBuffer* buffers, size_t bufferCount)
{
    QByteArray data;
    
    struct msghdr msgHdr = {0};
    struct sockaddr_in sin = {0};
    size_t sentLength = 0;

    memset (& msgHdr, 0, sizeof (struct msghdr));
    
    if (address != NULL)
    {
        memset (& sin, 0, sizeof (struct sockaddr_in));
        sin.sin_family = AF_INET;
        sin.sin_port = ENET_HOST_TO_NET_16 (address -> port);
        sin.sin_addr.s_addr = address -> host;

        msgHdr.msg_name = & sin;
        msgHdr.msg_namelen = sizeof (struct sockaddr_in);
    }
    data.append((char*)&sin, sizeof(struct sockaddr_in));
    
    for (int i = 0; i < bufferCount; i++) {
        const ENetBuffer *buffer = &buffers[i];
        sentLength += buffer->dataLength;
    }
    data.append((char*)&sentLength, sizeof(size_t));

    // 允许发送部分数据
    for (int i = 0; i < bufferCount; i++) {
        const ENetBuffer *buffer = &buffers[i];
        data.append((char*)buffer->data, buffer->dataLength);
    }

    // qDebug()<<sentLength<<bufferCount;
    int maxlen = TOX_MAX_MESSAGE_LENGTH - sizeof(struct sockaddr_in) - sizeof(size_t);
    if (sentLength > maxlen) {
        qDebug()<<address<<buffers<<bufferCount;
        qDebug()<<"warning, exceed tox max message length:"<<sentLength<<maxlen;
    }

    test_deserialize_packet(data);

    return data;    
}


size_t deserialize_packet(QByteArray pkt, ENetAddress *address, ENetBuffer *buffer)
{
    // deserialize
    struct sockaddr_in sin = {0};
    const char *pktdata = pkt.constData();
    memcpy(&sin, pktdata, sizeof(struct sockaddr_in));
    size_t length = 0;
    memcpy(&length, pktdata + sizeof(struct sockaddr_in), sizeof(size_t));
    int recvLength = 0;
    
    // qDebug()<<pkt.length()<<sizeof(struct sockaddr_in)<<sizeof(size_t)<<length;
    if (pkt.length() != (sizeof(struct sockaddr_in) + sizeof(size_t) + length)) {
        qDebug()<<"warning size not match excepted:"
                <<pkt.length()<<(sizeof(struct sockaddr_in) + sizeof(size_t) + length)
                <<sizeof(struct sockaddr_in)<<sizeof(size_t)<<length;
    }
    memcpy(buffer->data, pktdata + sizeof(struct sockaddr_in) + sizeof(size_t), length);
    buffer->dataLength = length;
    recvLength = length;

    if (address != NULL) {
        address->host = (enet_uint32) sin.sin_addr.s_addr;
        address->port = ENET_NET_TO_HOST_16 (sin.sin_port);
        // emu's

        sin.sin_family = AF_INET;
        // sin.sin_port = ENET_HOST_TO_NET_16 (7767);
        // sin.sin_addr.s_addr = inet_addr("127.0.0.1");

        address->host = (enet_uint32) sin.sin_addr.s_addr;
        address->port = ENET_NET_TO_HOST_16 (sin.sin_port);

        // qDebug()<<"got addr:";
    }

    return recvLength;
    return 0;
}

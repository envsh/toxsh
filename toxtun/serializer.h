#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <QtCore>

#include "enet/enet.h"


QByteArray serialize_packet(const ENetAddress * address, const ENetBuffer * buffer);
QByteArray serialize_packet(const ENetAddress* address, const ENetBuffer* buffers, size_t bufferCount);

size_t deserialize_packet(QByteArray pkt, ENetAddress *address, ENetBuffer *buffer);

#endif /* SERIALIZER_H */

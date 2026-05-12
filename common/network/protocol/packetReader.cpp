#include "packetReader.h"

#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>

PacketReader::PacketReader(const void *data, size_t size) : buffer(static_cast<const uint8_t *>(data)), size(size) {}

void PacketReader::readBytes(void *destination, size_t size)
{
    if (offset + size > this->size)
    {
        throw std::runtime_error("PacketReader overflow");
    }

    std::memcpy(destination, buffer + offset, size);

    offset += size;
}

uint8_t PacketReader::readUint8()
{

    uint8_t value;
    readBytes(&value, sizeof(value));

    return value;
}

uint16_t PacketReader::readUint16()
{

    uint16_t net;
    readBytes(&net, sizeof(net));

    return ntohs(net);
}

uint32_t PacketReader::readUint32()
{

    uint32_t net;
    readBytes(&net, sizeof(net));

    return ntohl(net);
}

std::string PacketReader::readString()
{

    uint16_t size = readUint16();
    std::string result(size, '\0');
    readBytes(result.data(), size);

    return result;
}

bool PacketReader::hasMore() const
{
    return offset < size;
}

size_t PacketReader::remaining() const
{
    return size - offset;
}
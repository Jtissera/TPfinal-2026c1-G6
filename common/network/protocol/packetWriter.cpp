#include "packetWriter.h"

void PacketWriter::writeBytes(const void *data, size_t size)
{
    auto ptr = static_cast<const uint8_t *>(data);
    buffer.insert(buffer.end(), ptr, ptr + size);
}

void PacketWriter::writeUint8(uint8_t value)
{
    writeBytes(&value, sizeof(value));
}

void PacketWriter::writeUint16(uint16_t value)
{
    uint16_t net = htons(value);
    writeBytes(&net, sizeof(net));
}

void PacketWriter::writeUint32(uint32_t value)
{
    uint32_t net = htonl(value);
    writeBytes(&net, sizeof(net));
}

void PacketWriter::writeString(const std::string &value)
{

    writeUint16(static_cast<uint16_t>(value.size()));
    writeBytes(value.data(), value.size());
}

const uint8_t *PacketWriter::data() const
{
    return buffer.data();
}

size_t PacketWriter::size() const
{
    return buffer.size();
}

void PacketWriter::clear()
{
    buffer.clear();
}
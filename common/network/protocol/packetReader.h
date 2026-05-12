#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class PacketReader
{
private:
    const uint8_t *buffer;
    size_t size;
    size_t offset = 0;

public:
    explicit PacketReader(const void *data, size_t size);

    void readBytes(void *destination, size_t size);

    uint8_t readUint8();
    uint16_t readUint16();
    uint32_t readUint32();

    std::string readString();

    bool hasMore() const;
    size_t remaining() const;
};
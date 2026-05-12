#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>
#include <string>

class PacketWriter
{
private:
    std::vector<uint8_t> buffer;

public:
    void writeBytes(const void *data, size_t size);

    void writeUint8(uint8_t value);

    void writeUint16(uint16_t value);

    void writeUint32(uint32_t value);

    void writeString(const std::string &value);

    const uint8_t *data() const;

    size_t size() const;

    void clear();
};
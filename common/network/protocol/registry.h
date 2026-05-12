#pragma once

#include <functional>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <stdexcept>

#include "../messages/message.h"
#include "packetReader.h"

using MessageFactory =
    std::function<std::unique_ptr<Message>(PacketReader &)>;

class Registry
{
public:
    void registerDeserializer(uint8_t opcode, MessageFactory factory);

    std::unique_ptr<Message> deserialize(
        uint8_t opcode, PacketReader &reader) const;

private:
    std::unordered_map<uint8_t, MessageFactory> factories;
};

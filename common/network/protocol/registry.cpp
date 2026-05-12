#include "registry.h"

void Registry::registerDeserializer(uint8_t opcode, MessageFactory factory)
{
    factories[opcode] = std::move(factory);
}

std::unique_ptr<Message> Registry::deserialize(uint8_t opcode, PacketReader &reader) const
{
    auto it = factories.find(opcode);

    if (it == factories.end())
    {
        throw std::runtime_error("Unknown opcode " + std::to_string(opcode));
    }

    return it->second(reader);
}
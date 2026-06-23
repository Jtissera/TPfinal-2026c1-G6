#pragma once
#include "../../message.h"
#include "../../../protocol/clientOpCode.h"
#include "../../../protocol/packetWriter.h"
#include <string>

class CreateCharMessage : public Message
{
public:
    CreateCharMessage(std::string name, std::string race, std::string characterClass);

    const std::string &getName() const;
    const std::string &getRace() const;
    const std::string &getCharacterClass() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    std::string name;
    std::string race;
    std::string characterClass;
};
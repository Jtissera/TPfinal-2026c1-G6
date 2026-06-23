#include "createCharMessage.h"

CreateCharMessage::CreateCharMessage(std::string name,
                                     std::string race,
                                     std::string characterClass)
    : name(std::move(name)),
      race(std::move(race)),
      characterClass(std::move(characterClass))
{
}

const std::string &CreateCharMessage::getName() const { return name; }
const std::string &CreateCharMessage::getRace() const { return race; }
const std::string &CreateCharMessage::getCharacterClass() const { return characterClass; }

uint8_t CreateCharMessage::opCode() const
{
    return static_cast<uint8_t>(ClientOpCode::MSG_CREATE_CHAR);
}

void CreateCharMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeString(name);
    writer.writeString(race);
    writer.writeString(characterClass);
}
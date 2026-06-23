#include "npcSpawnMessage.h"

NpcSpawnMessage::NpcSpawnMessage(uint32_t npcId,
                                 NpcType type,
                                 std::string name,
                                 uint16_t x,
                                 uint16_t y,
                                 uint16_t hp,
                                 uint16_t hpMax,
                                 uint16_t level,
                                 bool hostile)
    : npcId(npcId),
      type(type),
      name(std::move(name)),
      x(x),
      y(y),
      hp(hp),
      hpMax(hpMax),
      level(level),
      hostile(hostile)
{
}

uint32_t NpcSpawnMessage::getNpcId() const { return npcId; }
NpcType NpcSpawnMessage::getType() const { return type; }
const std::string &NpcSpawnMessage::getName() const { return name; }
uint16_t NpcSpawnMessage::getX() const { return x; }
uint16_t NpcSpawnMessage::getY() const { return y; }
uint16_t NpcSpawnMessage::getHp() const { return hp; }
uint16_t NpcSpawnMessage::getHpMax() const { return hpMax; }
uint16_t NpcSpawnMessage::getLevel() const { return level; }
bool NpcSpawnMessage::isHostile() const { return hostile; }

uint8_t NpcSpawnMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_NPC_SPAWN);
}

void NpcSpawnMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(npcId);
    writer.writeUint8(static_cast<uint8_t>(type));
    writer.writeString(name);
    writer.writeUint16(x);
    writer.writeUint16(y);
    writer.writeUint16(hp);
    writer.writeUint16(hpMax);
    writer.writeUint16(level);
    writer.writeUint8(hostile ? 1 : 0);
}
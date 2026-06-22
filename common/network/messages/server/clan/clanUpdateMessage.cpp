#include "clanUpdateMessage.h"

#include <utility>

ClanUpdateMessage::ClanUpdateMessage(uint32_t playerId,
                                     std::string clanName,
                                     bool isFounder)
    : playerId(playerId),
      clanName(std::move(clanName)),
      isFounder(isFounder)
{
}

uint8_t ClanUpdateMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_CLAN_UPDATE);
}

void ClanUpdateMessage::serializeBody(PacketWriter &writer) const
{
    // El orden debe coincidir con el deserializer del cliente.
    writer.writeUint32(playerId);
    writer.writeString(clanName);
    writer.writeUint8(isFounder ? 1 : 0);
}

uint32_t ClanUpdateMessage::getPlayerId() const
{
    return playerId;
}

const std::string &ClanUpdateMessage::getClanName() const
{
    return clanName;
}

bool ClanUpdateMessage::getIsFounder() const
{
    return isFounder;
}
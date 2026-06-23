#include "clanSyncMessage.h"

ClanSyncMessage::ClanSyncMessage(std::string clanName, bool isFounder)
    : clanName(std::move(clanName)),
      isFounder(isFounder)
{
}

const std::string &ClanSyncMessage::getClanName() const { return clanName; }
bool ClanSyncMessage::getIsFounder() const { return isFounder; }

uint8_t ClanSyncMessage::opCode() const
{
    return static_cast<uint8_t>(ClientOpCode::MSG_CLAN_SYNC_INTERNAL);
}

void ClanSyncMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeString(clanName);
    writer.writeUint8(isFounder ? 1 : 0);
}
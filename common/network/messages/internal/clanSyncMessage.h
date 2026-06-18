#pragma once

#include "common/network/messages/message.h"
#include "common/network/protocol/clientOpCode.h"
#include <string>

class ClanSyncMessage : public Message
{
public:
    ClanSyncMessage(std::string clanName, bool isFounder)
        : clanName(std::move(clanName)), isFounder(isFounder) {}

    const std::string &getClanName() const { return clanName; }
    bool getIsFounder() const { return isFounder; }

    uint8_t opCode() const override
    {
        return static_cast<uint8_t>(ClientOpCode::MSG_CLAN_SYNC_INTERNAL);
    }

    void serializeBody(PacketWriter &writer) const override
    {
        writer.writeString(clanName);
        writer.writeUint8(isFounder ? 1 : 0);
    }

private:
    std::string clanName;
    bool isFounder;
};
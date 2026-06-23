#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/clientOpCode.h"
#include "common/network/protocol/packetWriter.h"
#include <cstdint>
#include <string>

class ClanSyncMessage : public Message
{
public:
    ClanSyncMessage(std::string clanName, bool isFounder);

    const std::string &getClanName() const;
    bool getIsFounder() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    std::string clanName;
    bool isFounder;
};
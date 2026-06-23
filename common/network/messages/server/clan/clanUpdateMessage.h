#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"
#include <cstdint>
#include <string>

class ClanUpdateMessage : public Message
{
public:
    ClanUpdateMessage(uint32_t playerId, std::string clanName, bool isFounder);

    uint32_t getPlayerId() const;
    const std::string &getClanName() const;
    bool getIsFounder() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t playerId;
    std::string clanName;
    bool isFounder;
};
#ifndef CLAN_UPDATE_MESSAGE_H
#define CLAN_UPDATE_MESSAGE_H

#include <cstdint>
#include <string>

#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"

class ClanUpdateMessage : public Message
{
private:
    // ID del jugador cuyo clan cambió.
    uint32_t playerId;


    std::string clanName;

    // Indica si el jugador es fundador/líder del clan.
    bool isFounder;

public:

    ClanUpdateMessage(uint32_t playerId,std::string clanName,bool isFounder);
    
    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;
    uint32_t getPlayerId() const;
    const std::string &getClanName() const;
    bool getIsFounder() const;
};

#endif
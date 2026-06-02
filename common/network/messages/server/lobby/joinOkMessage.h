#pragma once

#include <cstdint>
#include <string>

#include "../../message.h"
#include "../../../protocol/serverOpCode.h"
#include "../../../protocol/packetWriter.h"

class JoinOkMessage : public Message {
public:
    JoinOkMessage(uint32_t gameId, std::string gameName,
                  uint16_t spawnX, uint16_t spawnY);

    uint32_t getGameId()       const;
    const std::string& getGameName() const;
    uint16_t getSpawnX()       const;
    uint16_t getSpawnY()       const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter& writer) const override;

private:
    uint32_t gameId;
    std::string gameName;
    uint16_t spawnX;
    uint16_t spawnY;
};

//Hay que ver si es elo mejor mandarlo por aca
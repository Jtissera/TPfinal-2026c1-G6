
#ifndef TALLER_TP_ENTITYSPAWNMESSAGE_H
#define TALLER_TP_ENTITYSPAWNMESSAGE_H

#pragma once

#include <cstdint>
#include "../../message.h"
#include "../../../protocol/serverOpCode.h"
#include "../../../protocol/packetWriter.h"
#include "common/dtos/gameTypes.h"

class EntitySpawnMessage : public Message {
private:
    PlayerDto playerDto;

public:
    explicit EntitySpawnMessage(PlayerDto playerDto);

    const PlayerDto& getPlayerDto() const;

    uint8_t opCode() const override;

    void serializeBody(PacketWriter& writer) const override;
};

#endif //TALLER_TP_ENTITYSPAWNMESSAGE_H

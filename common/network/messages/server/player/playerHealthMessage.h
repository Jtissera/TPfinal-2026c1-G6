

#ifndef TALLER_TP_PLAYERHEALTHMESSAGE_H
#define TALLER_TP_PLAYERHEALTHMESSAGE_H

#include <cstdint>
#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"
class PlayerHealthMessage : public Message {

private:
    uint32_t playerId;
    uint16_t hp;
    uint16_t hpMax;
public:

    explicit PlayerHealthMessage(uint32_t playerId,uint16_t hp, uint16_t hpMax);
    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

    uint32_t getPlayerId() const;
    uint16_t getHp() const ;
    uint16_t getHpMax() const;
};
#endif //TALLER_TP_PLAYERHEALTHMESSAGE_H

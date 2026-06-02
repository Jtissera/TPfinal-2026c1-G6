//
// Created by mauro on 1/6/26.
//

#ifndef TALLER_TP_ENEMYHITPLAYERMESSAGE_H
#define TALLER_TP_ENEMYHITPLAYERMESSAGE_H
#include "common/network/messages/message.h"


class EnemyHitPlayerMessage : public Message {

public:
    explicit EnemyHitPlayerMessage(uint32_t enemyId);
    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

    uint32_t getEnemyId() const;

private:
    uint32_t enemyId;
};

#endif //TALLER_TP_ENEMYHITPLAYERMESSAGE_H

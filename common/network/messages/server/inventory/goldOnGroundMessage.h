
#ifndef TALLER_TP_GOLDONGROUNDMESSAGE_H
#define TALLER_TP_GOLDONGROUNDMESSAGE_H

#pragma once
#include "common/network/messages/message.h"

class GoldOnGroundMessage : public Message {
    uint32_t amount;
    int x, y;
public:
    explicit GoldOnGroundMessage(uint32_t amount, int x, int y);

    uint32_t getAmount() const;
    int getX() const;
    int getY() const;

    uint8_t opCode() const override;

    void serializeBody(PacketWriter& writer) const override;
};
#endif //TALLER_TP_GOLDONGROUNDMESSAGE_H

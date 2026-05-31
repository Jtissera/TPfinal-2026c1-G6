
#ifndef TALLER_TP_ATTACKMESSAGE_H
#define TALLER_TP_ATTACKMESSAGE_H
#include "common/network/messages/message.h"


class AttackMessage : public Message {


public:
    explicit AttackMessage(uint32_t targetId);
    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t targetId;

};



#endif //TALLER_TP_ATTACKMESSAGE_H

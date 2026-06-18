
#ifndef TALLER_TP_PICKITEMMESSAGE_H
#define TALLER_TP_PICKITEMMESSAGE_H

#include "common/network/messages/message.h"
#include "common/network/protocol/clientOpCode.h"

class PickItemMessage : public Message {
    uint32_t instanceId;
    bool isGold;
public:
    explicit PickItemMessage(uint32_t instanceId, bool isGold);
    uint32_t getInstanceId() const;
    bool getIsGold() const;
    uint8_t opCode() const override;
    void serializeBody(PacketWriter& writer) const override;
};

#endif //TALLER_TP_PICKITEMMESSAGE_H
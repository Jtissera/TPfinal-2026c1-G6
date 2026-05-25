
#ifndef TALLER_TP_ENTITYMOVEMESSAGE_H
#define TALLER_TP_ENTITYMOVEMESSAGE_H

#include <cstdint>
#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"

class EntityMoveMessage : public Message {
public:
    EntityMoveMessage(uint8_t id, int16_t x, int16_t y);

    uint8_t opCode() const override;


    void serializeBody(PacketWriter& writer) const override;


    uint8_t getId() const;
    int16_t getX()   const ;
    int16_t getY()   const;
private:
    std::uint8_t entityId;
    uint16_t x;
    uint16_t y;
};



#endif //TALLER_TP_ENTITYMOVEMESSAGE_H

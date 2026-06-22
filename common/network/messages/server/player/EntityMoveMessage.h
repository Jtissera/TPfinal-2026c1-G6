
#ifndef TALLER_TP_ENTITYMOVEMESSAGE_H
#define TALLER_TP_ENTITYMOVEMESSAGE_H

#include <cstdint>
#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"
#include "common/dtos/gameTypes.h"

class EntityMoveMessage : public Message {
public:
    EntityMoveMessage(uint32_t id, int16_t x, int16_t y,Direction direction,bool moving);

    uint8_t opCode() const override;


    void serializeBody(PacketWriter& writer) const override;


    uint32_t getId() const;
    int16_t getX()   const ;
    int16_t getY()   const;

    // Dirección aceptada por el server.
    Direction getDirection() const;
    // Indica si debe reproducir animación de caminar.
    bool isMoving() const;

private:
    std::uint32_t entityId;
    uint16_t x;
    uint16_t y;
    Direction direction;
    bool moving;
};



#endif //TALLER_TP_ENTITYMOVEMESSAGE_H

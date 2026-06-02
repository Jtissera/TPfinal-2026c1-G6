
#ifndef TALLER_TP_MOVEMESSAGE_H
#define TALLER_TP_MOVEMESSAGE_H


#include "../../message.h"
#include "../../../protocol/clientOpCode.h"
#include "../../../protocol/packetWriter.h"
#include "../../../../dtos/gameTypes.h"


class MoveMessage : public Message {

public:

    explicit MoveMessage (Direction dir);
    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;
    Direction getDirection() const;
private:
    Direction dir;
};







#endif //TALLER_TP_MOVEMESSAGE_H
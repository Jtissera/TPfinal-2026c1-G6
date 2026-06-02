#include "playerDiedMessage.h"

#include "common/network/protocol/serverOpCode.h"


PlayerDiedMessage::PlayerDiedMessage(uint32_t id) : id(id) {}


uint8_t PlayerDiedMessage::opCode() const {
    return static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_DIED);
}


void PlayerDiedMessage::serializeBody(PacketWriter& writer) const {
    writer.writeUint32(id);
}

// Devuelve el id del jugador muerto.
// Lo usa el cliente después de deserializar el mensaje.
uint32_t PlayerDiedMessage::getId() const {
    return id;
}

#include "EntitySpawnMessage.h"

EntitySpawnMessage::EntitySpawnMessage(PlayerDto playerDto)
    : playerDto(std::move(playerDto)) {
}

const PlayerDto& EntitySpawnMessage::getPlayerDto() const {
    return playerDto;
}

uint8_t EntitySpawnMessage::opCode() const {
    return static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_SPAWN);
}

void EntitySpawnMessage::serializeBody(PacketWriter& writer) const {
    // Mismo formato que JoinOkMessage para PlayerDto, pero sin datos de partida.

    writer.writeString(playerDto.nombre);
    writer.writeUint8(playerDto.playerID);
    writer.writeString(playerDto.raza);
    writer.writeString(playerDto.clase);

    writer.writeUint32(static_cast<uint32_t>(playerDto.headId));
    writer.writeUint8(playerDto.level);

    writer.writeUint16(static_cast<uint16_t>(playerDto.hp));
    writer.writeUint16(static_cast<uint16_t>(playerDto.mana));
    writer.writeUint16(static_cast<uint16_t>(playerDto.hpMax));
    writer.writeUint16(static_cast<uint16_t>(playerDto.manaMax));

    writer.writeUint32(static_cast<uint32_t>(playerDto.oro));
    writer.writeUint32(static_cast<uint32_t>(playerDto.oroMax));

    writer.writeUint16(playerDto.xpos);
    writer.writeUint16(playerDto.ypos);

    writer.writeUint32(static_cast<uint32_t>(playerDto.exp));
    writer.writeUint32(static_cast<uint32_t>(playerDto.expMax));

    writer.writeUint8(playerDto.esFantasma ? 1 : 0);

    writer.writeUint32(static_cast<uint32_t>(playerDto.fuerza));
    writer.writeUint32(static_cast<uint32_t>(playerDto.agilidad));
    writer.writeUint32(static_cast<uint32_t>(playerDto.inteligencia));
    writer.writeUint32(static_cast<uint32_t>(playerDto.constitucion));
}
#include "joinOkMessage.h"

JoinOkMessage::JoinOkMessage(uint32_t gameId, std::string gameName,PlayerDto playerDto)
    : gameId(gameId), gameName(std::move(gameName)),playerDto(std::move(playerDto)) {}

uint32_t JoinOkMessage::getGameId() const
{
    return gameId;
}

const std::string &JoinOkMessage::getGameName() const
{
    return gameName;
}

const PlayerDto& JoinOkMessage::getPlayerDto() const
{
    return playerDto;
}

uint8_t JoinOkMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_JOIN_OK);
}

void JoinOkMessage::serializeBody(PacketWriter &writer) const
{
    // Datos de la partida.
    writer.writeUint32(gameId);
    writer.writeString(gameName);

    // Datos completos del jugador.
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
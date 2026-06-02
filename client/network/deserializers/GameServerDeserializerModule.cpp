#include "GameServerDeserializerModule.h"

#include "common/network/messages/server/world/EntitySpawnMessage.h"


void GameServerDeserializersModule::registerDeserializers(Registry& registry) const {
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_MOVE),
        [](PacketReader& reader) -> std::unique_ptr<Message> {
            auto id = reader.readUint8();
            auto x  = static_cast<int16_t>(reader.readUint16());
            auto y  = static_cast<int16_t>(reader.readUint16());

            auto direction = static_cast<Direction>(reader.readUint8());
            bool moving = reader.readUint8() != 0;

            return std::make_unique<EntityMoveMessage>(
                id,
                x,
                y,
                direction,
                moving
            );
        });

    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_STATS),
        [](PacketReader& reader) -> std::unique_ptr<Message> {
            auto level    = reader.readUint8();
            auto hp       = static_cast<int16_t>(reader.readUint16());
            auto maxHp    = static_cast<int16_t>(reader.readUint16());
            auto mana     = static_cast<int16_t>(reader.readUint16());
            auto maxMana  = static_cast<int16_t>(reader.readUint16());
            auto exp      = reader.readUint32();
            auto expLimit = reader.readUint32();
            auto gold     = reader.readUint32();
            return std::make_unique<PlayerStatsMessage>(
                level, hp, maxHp, mana, maxMana, exp, expLimit, gold);
        });

    registry.registerDeserializer(
    static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_DIED),
    [](PacketReader& reader) -> std::unique_ptr<Message> {
        uint32_t id = reader.readUint32();
        return std::make_unique<PlayerDiedMessage>(id);
    });
    registry.registerDeserializer(
    static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_SPAWN),
    [](PacketReader& reader) -> std::unique_ptr<Message> {
        PlayerDto dto;

        dto.nombre = reader.readString();
        dto.playerID = reader.readUint8();
        dto.raza = reader.readString();
        dto.clase = reader.readString();

        dto.headId = static_cast<int>(reader.readUint32());
        dto.level = reader.readUint8();

        dto.hp = reader.readUint16();
        dto.mana = reader.readUint16();
        dto.hpMax = reader.readUint16();
        dto.manaMax = reader.readUint16();

        dto.oro = reader.readUint32();
        dto.oroMax = reader.readUint32();

        dto.xpos = reader.readUint16();
        dto.ypos = reader.readUint16();

        dto.exp = reader.readUint32();
        dto.expMax = reader.readUint32();

        dto.esFantasma = reader.readUint8() != 0;

        dto.fuerza = reader.readUint32();
        dto.agilidad = reader.readUint32();
        dto.inteligencia = reader.readUint32();
        dto.constitucion = reader.readUint32();

        return std::make_unique<EntitySpawnMessage>(std::move(dto));
    }
);
}

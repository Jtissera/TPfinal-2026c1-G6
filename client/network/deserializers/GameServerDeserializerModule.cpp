#include "GameServerDeserializerModule.h"

void GameServerDeserializersModule::registerDeserializers(Registry& registry) const {

    // ── Movimiento ────────────────────────────────────────────────────────────
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_MOVE),
        [](PacketReader& reader) -> std::unique_ptr<Message> {
            auto id = reader.readUint8();
            auto x  = static_cast<int16_t>(reader.readUint16());
            auto y  = static_cast<int16_t>(reader.readUint16());
            return std::make_unique<EntityMoveMessage>(id, x, y);
        });

    // ── Stats del jugador ─────────────────────────────────────────────────────
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

    // ── Muerte del jugador ────────────────────────────────────────────────────
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_DIED),
        [](PacketReader& reader) -> std::unique_ptr<Message> {
            auto id = reader.readUint32();
            return std::make_unique<PlayerDiedMessage>(id);
        });

    // ── Level up ──────────────────────────────────────────────────────────────
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_LEVEL_UP),
        [](PacketReader& reader) -> std::unique_ptr<Message> {
            auto level = reader.readUint8();
            return std::make_unique<LevelUpMessage>(level);
        });

    // ── Inventario actualizado ────────────────────────────────────────────────
    // Wire format (servidor): uint8 count, luego por cada ítem: uint32 id,
    // string typeName, uint8 slot. Después EQUIP_SLOT_COUNT uint32 equipados.
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_INVENTORY_UPDATE),
        [](PacketReader& reader) -> std::unique_ptr<Message> {
            uint8_t count = reader.readUint8();
            std::vector<ItemDto> items;
            items.reserve(count);
            for (uint8_t i = 0; i < count; ++i) {
                ItemDto dto;
                dto.id       = reader.readUint32();
                dto.typeName = reader.readString();
                dto.slot     = reader.readUint8();
                items.push_back(std::move(dto));
            }
            std::array<uint32_t, EQUIP_SLOT_COUNT> equipped{};
            for (auto& slot : equipped)
                slot = reader.readUint32();
            return std::make_unique<InventoryUpdateMessage>(
                std::move(items), equipped);
        });

    // ── Ítem en el suelo ──────────────────────────────────────────────────────
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_ITEM_ON_GROUND),
        [](PacketReader& reader) -> std::unique_ptr<Message> {
            ItemDto dto;
            dto.id       = reader.readUint32();
            dto.typeName = reader.readString();
            auto x = static_cast<int>(reader.readUint16());
            auto y = static_cast<int>(reader.readUint16());
            return std::make_unique<ItemOnGroundMessage>(std::move(dto), x, y);
        });

    // ── Ítem recogido ─────────────────────────────────────────────────────────
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_ITEM_PICKED),
        [](PacketReader& reader) -> std::unique_ptr<Message> {
            auto clientId = reader.readUint32();
            auto itemId   = reader.readUint32();
            return std::make_unique<ItemPickedMessage>(clientId, itemId);
        });

    // ── Log de combate ────────────────────────────────────────────────────────
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_COMBAT_LOG),
        [](PacketReader& reader) -> std::unique_ptr<Message> {
            auto text = reader.readString();
            return std::make_unique<CombatLogMessage>(std::move(text));
        });

    // ── HP de NPC ─────────────────────────────────────────────────────────────
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_NPC_HEALTH),
        [](PacketReader& reader) -> std::unique_ptr<Message> {
            auto npcId = reader.readUint32();
            auto hp    = static_cast<int16_t>(reader.readUint16());
            auto maxHp = static_cast<int16_t>(reader.readUint16());
            return std::make_unique<NpcHealthMessage>(npcId, hp, maxHp);
        });

        // ── Lista inicial de NPCs ─────────────────────────────────────────────────
registry.registerDeserializer(
    static_cast<uint8_t>(ServerOpCode::MSG_NPC_LIST),
    [](PacketReader& reader) -> std::unique_ptr<Message> {
        uint16_t count = reader.readUint16();
        std::vector<NpcSnapshot> npcs;
        npcs.reserve(count);
        for (uint16_t i = 0; i < count; ++i) {
            NpcSnapshot s;
            s.id   = reader.readUint32();
            s.type = static_cast<NpcType>(reader.readUint8());
            s.x    = reader.readUint16();
            s.y    = reader.readUint16();
            npcs.push_back(s);
        }
        return std::make_unique<NpcListMessage>(std::move(npcs));
    });

// ── Spawn de NPC ──────────────────────────────────────────────────────────
registry.registerDeserializer(
    static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_SPAWN),
    [](PacketReader& reader) -> std::unique_ptr<Message> {
        auto id   = reader.readUint32();
        auto type = static_cast<NpcType>(reader.readUint8());
        auto x    = reader.readUint32();  // ← cambiar uint16 → uint32
        auto y    = reader.readUint32();  // ← cambiar uint16 → uint32
        return std::make_unique<EntitySpawnMessage>(id, type, x, y);
    });
// ── Despawn de NPC ────────────────────────────────────────────────────────
registry.registerDeserializer(
    static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_DESPAWN),
    [](PacketReader& reader) -> std::unique_ptr<Message> {
        auto id = reader.readUint32();
        return std::make_unique<EntityDespawnMessage>(id);
    });
}
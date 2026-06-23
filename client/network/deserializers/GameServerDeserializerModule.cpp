#include "GameServerDeserializerModule.h"
#include "common/network/messages/server/player/playerResurrectedMessage.h"

#include <iostream>

#include "common/network/messages/client/inventory/useItemMessage.h"
#include "common/network/messages/server/inventory/goldOnGroundMessage.h"
#include "common/network/messages/server/world/EntitySpawnMessage.h"
#include "common/network/messages/server/inventory/inventoryUpdateMessage.h"
#include "common/network/messages/server/inventory/itemOnGroundMessage.h"
#include "common/network/messages/server/inventory/itemPickedMessage.h"
#include "common/network/messages/server/player/levelUpMessage.h"
#include "common/network/messages/server/player/playerEquipmentUpdateMessage.h"
#include "common/network/messages/server/npc/npcSpawnMessage.h"
#include "common/network/messages/server/npc/npcHealthMessage.h"
#include "common/network/messages/server/npc/npcMoveMessage.h"
#include "common/network/messages/server/player/EntityDespawnMessage.h"
#include "common/network/messages/server/chat/chatNotificationMessage.h"
#include "common/network/messages/server/clan/clanUpdateMessage.h"
#include "common/network/messages/server/player/resurrectionStartedMessage.h"
#include "common/network/messages/server/combat/combatLogMessage.h"
#include "common/network/protocol/clientOpCode.h"
#include "common/network/messages/server/npc/npcAttackMessage.h"
#include "common/network/messages/server/player/playerAttackVisualMessage.h"
#include "common/network/messages/server/player/playerHealthMessage.h"

void GameServerDeserializersModule::registerDeserializers(Registry &registry) const
{
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_MOVE),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            auto id = reader.readUint32();
            auto x = static_cast<int16_t>(reader.readUint16());
            auto y = static_cast<int16_t>(reader.readUint16());

            auto direction = static_cast<Direction>(reader.readUint8());
            bool moving = reader.readUint8() != 0;

            return std::make_unique<EntityMoveMessage>(
                id,
                x,
                y,
                direction,
                moving);
        });

    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_STATS),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            auto level = reader.readUint8();
            auto hp = static_cast<int16_t>(reader.readUint16());
            auto maxHp = static_cast<int16_t>(reader.readUint16());
            auto mana = static_cast<int16_t>(reader.readUint16());
            auto maxMana = static_cast<int16_t>(reader.readUint16());
            auto exp = reader.readUint32();
            auto expLimit = reader.readUint32();
            auto gold = reader.readUint32();
            return std::make_unique<PlayerStatsMessage>(
                level, hp, maxHp, mana, maxMana, exp, expLimit, gold);
        });

    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_DIED),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            uint32_t id = reader.readUint32();
            return std::make_unique<PlayerDiedMessage>(id);
        });
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_SPAWN),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            PlayerDto dto;

            dto.nombre = reader.readString();
            dto.playerID = reader.readUint32();
            dto.raza = reader.readString();
            dto.clase = reader.readString();
            dto.clanName = reader.readString();

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
        });
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_INVENTORY_UPDATE),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            const uint8_t itemCount = reader.readUint8();

            std::vector<Item> items;
            items.reserve(itemCount);

            for (uint8_t i = 0; i < itemCount; ++i)
            {
                Item item{};

                item.instanceId = reader.readUint32();
                item.catalogId = reader.readUint32();
                item.typeName = reader.readString();
                item.slot = static_cast<ItemSlot>(reader.readUint8());

                items.push_back(std::move(item));
            }
            const uint8_t slotCount = reader.readUint8();

            std::array<uint32_t, InventoryUpdateMessage::INVENTORY_SLOT_COUNT> inventorySlots{};

            for (uint8_t i = 0; i < slotCount; ++i)
            {
                const uint32_t itemInstanceId = reader.readUint32();

                if (i < inventorySlots.size())
                {
                    inventorySlots[i] = itemInstanceId;
                }
            }

            std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)> equipped{};

            for (std::size_t i = 0; i < equipped.size(); ++i)
            {
                equipped[i] = reader.readUint32();
            }

            return std::make_unique<InventoryUpdateMessage>(
                std::move(items),
                inventorySlots,
                equipped);
        });
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_EQUIPMENT_UPDATE),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            const uint32_t playerId = reader.readUint32();

            EquipmentDto equipment{};
            equipment.weaponCatalogId = reader.readUint32();
            equipment.armorCatalogId = reader.readUint32();
            equipment.helmetCatalogId = reader.readUint32();
            equipment.shieldCatalogId = reader.readUint32();

            return std::make_unique<PlayerEquipmentUpdateMessage>(
                playerId,
                equipment);
        });
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_LEVEL_UP),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            std::cout << "[DESERIALIZER] MSG_LEVEL_UP leyendo playerId + level"
                      << std::endl;

            // Debe coincidir con serializeBody().
            const uint32_t playerId = reader.readUint32();
            const uint8_t newLevel = reader.readUint8();

            return std::make_unique<LevelUpMessage>(playerId, newLevel);
        });
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_RESURRECTED),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            const uint32_t playerId = reader.readUint32();
            const uint16_t tileX = reader.readUint16();
            const uint16_t tileY = reader.readUint16();
            return std::make_unique<PlayerResurrectedMessage>(playerId, tileX, tileY);
        });
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_NPC_SPAWN),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            const uint32_t npcId = reader.readUint32();
            const NpcType type = static_cast<NpcType>(reader.readUint8());
            const std::string name = reader.readString();
            const uint16_t x = reader.readUint16();
            const uint16_t y = reader.readUint16();
            const uint16_t hp = reader.readUint16();
            const uint16_t hpMax = reader.readUint16();

            // Nuevo campo: debe leerse en el mismo orden en que el server lo escribe.
            const uint16_t level = reader.readUint16();

            const bool hostile = reader.readUint8() != 0;

            return std::make_unique<NpcSpawnMessage>(
                npcId,
                type,
                name,
                x,
                y,
                hp,
                hpMax,
                level,
                hostile);
        });
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_NPC_HEALTH),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            const uint32_t npcId = reader.readUint32();
            const uint16_t hp = reader.readUint16();
            const uint16_t maxHp = reader.readUint16();

            return std::make_unique<NpcHealthMessage>(
                npcId,
                hp,
                maxHp);
        });
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_NPC_MOVE),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            // Leemos en el mismo orden en que serializa NpcMoveMessage.
            const uint32_t npcId = reader.readUint32();
            const uint16_t x = reader.readUint16();
            const uint16_t y = reader.readUint16();

            return std::make_unique<NpcMoveMessage>(npcId, x, y);
        });

    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_MAP_CHANGED),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            std::string mapPath = reader.readString();
            std::cout << "[CLIENT PROTOCOL] Deserializado MSG_MAP_CHANGED con path: " << mapPath << std::endl;
            return std::make_unique<MapChangedMessage>(std::move(mapPath));
        });

    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_DESPAWN),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            const uint32_t id = reader.readUint32();
            return std::make_unique<EntityDespawnMessage>(id);
        });
    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_ERROR),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            std::string errorMsg = reader.readString();
            std::cout << "[CLIENT PROTOCOL] Deserializado MSG_ERROR: " << errorMsg << std::endl;
            return std::make_unique<ErrorMessage>(std::move(errorMsg));
        });

    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_CHAT_MESSAGE),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            std::string text = reader.readString();
            auto type = static_cast<ChatMsgType>(reader.readUint8());
            return std::make_unique<ChatNotificationMessage>(
                std::move(text), type);
        });

    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_RESURRECTION_STARTED),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            const uint32_t delayMs = reader.readUint32();
            return std::make_unique<ResurrectionStartedMessage>(delayMs);
        });

    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_COMBAT_LOG),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            std::string text = reader.readString();
            return std::make_unique<CombatLogMessage>(std::move(text));
        });

    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_GOLD_ON_GROUND),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            const uint32_t instanceId = reader.readUint32();
            const uint32_t amount = reader.readUint32();
            const int x = reader.readUint16();
            const int y = reader.readUint16();
            return std::make_unique<GoldOnGroundMessage>(instanceId,amount, x, y);
        });

    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_ITEM_ON_GROUND),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            Item item{};
            item.instanceId = reader.readUint32();
            item.catalogId = reader.readUint32();
            item.typeName = reader.readString();

            const int x = reader.readUint16();
            const int y = reader.readUint16();

            return std::make_unique<ItemOnGroundMessage>(item, x, y);
        });

    registry.registerDeserializer(
        static_cast<uint8_t>(ServerOpCode::MSG_ITEM_PICKED),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            const uint32_t clientId = reader.readUint32();
            const uint32_t itemId = reader.readUint32();
            return std::make_unique<ItemPickedMessage>(clientId, itemId);
        });

    registry.registerDeserializer(
    static_cast<uint8_t>(ServerOpCode::MSG_NPC_ATTACK),
    [](PacketReader &reader) -> std::unique_ptr<Message>
    {
        const uint32_t npcId = reader.readUint32();
        const auto direction = static_cast<Direction>(reader.readUint8());
        return std::make_unique<NpcAttackMessage>(npcId, direction);
    });

    registry.registerDeserializer(static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_ATTACK_VISUAL),
[](PacketReader &reader) -> std::unique_ptr<Message>
    {
        const uint32_t attackerId = reader.readUint32();
        const uint32_t targetId = reader.readUint32();

        const auto visualType = static_cast<PlayerAttackVisualType>(reader.readUint8());
        std::string effectId = reader.readString();

        return std::make_unique<PlayerAttackVisualMessage>(attackerId,targetId,visualType,effectId);
    });
    registry.registerDeserializer(static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_HEALTH),
[](PacketReader &reader) -> std::unique_ptr<Message>
    {
        const uint32_t playerId = reader.readUint32();
        const uint16_t hp = reader.readUint16();
        const uint16_t hpMax = reader.readUint16();
        return std::make_unique<PlayerHealthMessage>(playerId,hp,hpMax);
    });
    registry.registerDeserializer(
    static_cast<uint8_t>(ServerOpCode::MSG_CLAN_UPDATE),
    [](PacketReader &reader) -> std::unique_ptr<Message>
    {
        const uint32_t playerId = reader.readUint32();
        std::string clanName = reader.readString();
        const bool isFounder = reader.readUint8() != 0;

        return std::make_unique<ClanUpdateMessage>(
            playerId,
            clanName,
            isFounder
        );
    });

}


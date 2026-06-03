
#include "GameClientDeserializersModule.h"

#include "common/dtos/gameTypes.h"
#include "common/network/messages/client/inventory/equipItemMessage.h"
#include "common/network/messages/client/movement/moveMessage.h"
#include "common/network/protocol/clientOpCode.h"
#include "common/network/messages/client/inventory/unequipSlotMessage.h"

void GameClientDeserializersModule::registerDeserializers(Registry& registry) const {
    registry.registerDeserializer(
        static_cast<uint8_t>(ClientOpCode::MSG_MOVE),
        [](PacketReader& reader) -> std::unique_ptr<Message> {
            // Primero leemos la dirección enviada por el cliente.
            auto direction = static_cast<Direction>(reader.readUint8());

            // Después leemos si el jugador está caminando o se detuvo.
            // 1 = moving, 0 = stopped.
            bool moving = reader.readUint8() != 0;
            // Construimos el mensaje con el nuevo formato.
            return std::make_unique<MoveMessage>(direction, moving);
        });

    registry.registerDeserializer(
    static_cast<uint8_t>(ClientOpCode::MSG_ENEMY_HIT_PLAYER),
    [](PacketReader& reader) -> std::unique_ptr<Message> {
        uint32_t enemyId = reader.readUint32();
        return std::make_unique<EnemyHitPlayerMessage>(enemyId);
    });
    registry.registerDeserializer(
    static_cast<uint8_t>(ClientOpCode::MSG_RESURRECT),
    [](PacketReader& reader) -> std::unique_ptr<Message> {
        (void)reader;
        return std::make_unique<ResurrectMessage>();
    });
    registry.registerDeserializer(
    static_cast<uint8_t>(ClientOpCode::MSG_EQUIP_ITEM),
    [](PacketReader& reader) -> std::unique_ptr<Message> {
        const uint32_t itemInstanceId = reader.readUint32();

        return std::make_unique<EquipItemMessage>(itemInstanceId);
    });
    registry.registerDeserializer(
    static_cast<uint8_t>(ClientOpCode::MSG_UNEQUIP_SLOT),
    [](PacketReader& reader) -> std::unique_ptr<Message> {
        const auto slot = static_cast<EquipSlot>(reader.readUint8());
        return std::make_unique<UnequipSlotMessage>(slot);
    });
}


#include "GameClientDeserializersModule.h"

#include "common/dtos/gameTypes.h"
#include "common/network/messages/client/combat/attackMessage.h"
#include "common/network/messages/client/inventory/equipItemMessage.h"
#include "common/network/messages/client/cheat/cheatMessage.h"
#include "common/network/messages/client/city/interactNpcMessage.h"
#include "common/network/messages/client/inventory/pickItemMessage.h"
#include "common/network/messages/client/movement/moveMessage.h"
#include "common/network/protocol/clientOpCode.h"
#include "common/network/messages/client/inventory/unequipSlotMessage.h"
#include "common/network/messages/client/inventory/useItemMessage.h"
#include "common/network/messages/client/chat/chatMessage.h"
#include "server/game/items/EquipSlot.h"

void GameClientDeserializersModule::registerDeserializers(Registry &registry) const
{
    registry.registerDeserializer(
        static_cast<uint8_t>(ClientOpCode::MSG_MOVE),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            // Primero leemos la dirección enviada por el cliente.
            auto direction = static_cast<Direction>(reader.readUint8());

            // Después leemos si el jugador está caminando o se detuvo.
            // 1 = moving, 0 = stopped.
            bool moving = reader.readUint8() != 0;
            // Construimos el mensaje con el nuevo formato.
            return std::make_unique<MoveMessage>(direction, moving);
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
    registry.registerDeserializer(
    static_cast<uint8_t>(ClientOpCode::MSG_USE_ITEM),
    [](PacketReader& reader) -> std::unique_ptr<Message> {
        const uint32_t itemInstanceId = reader.readUint32();

        return std::make_unique<UseItemMessage>(itemInstanceId);
    });
    registry.registerDeserializer(
    static_cast<uint8_t>(ClientOpCode::MSG_ATTACK),
    [](PacketReader& reader) -> std::unique_ptr<Message> {
        const uint32_t targetId = reader.readUint32();

        return std::make_unique<AttackMessage>(targetId);
    });

  registry.registerDeserializer(
      static_cast<uint8_t>(ClientOpCode::MSG_CHEAT),
      [](PacketReader &reader) -> std::unique_ptr<Message>
      {
        auto cheat = static_cast<CheatType>(reader.readUint8());
        return std::make_unique<CheatMessage>(cheat);
      });

    registry.registerDeserializer(
        static_cast<uint8_t>(ClientOpCode::MSG_INTERACT_NPC),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            auto npcId = reader.readUint32();
            auto cmd = reader.readString();
            return std::make_unique<InteractNpcMessage>(npcId, std::move(cmd));
        });

    registry.registerDeserializer(
        static_cast<uint8_t>(ClientOpCode::MSG_CHAT),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
            std::string text = reader.readString();
            uint32_t targetId = reader.readUint32();
            return std::make_unique<ChatMessage>(std::move(text), targetId);
        });

    registry.registerDeserializer(
        static_cast<uint8_t>(ClientOpCode::MSG_PICK_ITEM),
        [](PacketReader &reader) -> std::unique_ptr<Message>
        {
          const uint32_t instanceId = reader.readUint32();
          const bool isGold = reader.readUint8() != 0;
          return std::make_unique<PickItemMessage>(instanceId, isGold);
        });
    registry.registerDeserializer(
    static_cast<uint8_t>(ClientOpCode::MSG_CHEAT),
    [](PacketReader &reader) -> std::unique_ptr<Message>
    {
      auto cheat = static_cast<CheatType>(reader.readUint8());
      return std::make_unique<CheatMessage>(cheat);
    });
}
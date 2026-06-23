#pragma once

#include "../../../common/network/deserializerModule.h"
#include "../../../common/network/messages/client/chat/chatMessage.h"
#include "../../../common/network/messages/client/cheat/cheatMessage.h"
#include "../../../common/network/messages/client/city/interactNpcMessage.h"
#include "../../../common/network/messages/client/combat/attackMessage.h"
#include "../../../common/network/messages/client/combat/resurrectMessage.h"
#include "../../../common/network/messages/client/inventory/equipItemMessage.h"
#include "../../../common/network/messages/client/inventory/pickItemMessage.h"
#include "../../../common/network/messages/client/inventory/unequipSlotMessage.h"
#include "../../../common/network/messages/client/inventory/useItemMessage.h"
#include "../../../common/network/messages/client/movement/moveMessage.h"
#include "../../../common/network/protocol/clientOpCode.h"
#include "../../../common/network/protocol/registry.h"
#include "../../../common/dtos/gameTypes.h"
#include "../../../server/game/items/EquipSlot.h"

class GameClientDeserializersModule : public DeserializerModule
{
public:
    void registerDeserializers(Registry &registry) const override;
};
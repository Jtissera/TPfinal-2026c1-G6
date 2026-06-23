#pragma once

#include <cstdint>
#include <memory>

#include "common/network/messages/message.h"
#include "server/monitorQueues.h"
#include "server/world/gameWorld.h"
#include "server/game/stats/gameFormulas.h"

#include "common/network/messages/client/inventory/dropItemMessage.h"
#include "common/network/messages/client/inventory/equipItemMessage.h"
#include "common/network/messages/client/inventory/pickItemMessage.h"
#include "common/network/messages/client/inventory/unequipSlotMessage.h"
#include "common/network/messages/client/inventory/useItemMessage.h"
#include "common/network/messages/server/inventory/goldOnGroundMessage.h"
#include "common/network/messages/server/inventory/inventoryUpdateMessage.h"
#include "common/network/messages/server/inventory/itemPickedMessage.h"
#include "common/network/messages/server/player/playerEquipmentUpdateMessage.h"
#include "common/network/messages/server/player/playerStatsMessage.h"
#include "server/game/equipmentDtoFactory.h"

class InventoryHandler
{
public:
    explicit InventoryHandler(const toml::table &config);

    void handlePickItem(uint32_t clientId,
                        const Message &msg,
                        GameWorld &world,
                        Monitor &monitor);

    void handleDropItem(uint32_t clientId,
                        const Message &msg,
                        GameWorld &world,
                        Monitor &monitor);

    void handleEquipItem(uint32_t clientId,
                         const Message &msg,
                         GameWorld &world,
                         Monitor &monitor);

    void handleUnequipSlot(uint32_t clientId,
                           const Message &msg,
                           GameWorld &world,
                           Monitor &monitor);

    void handleUseItem(uint32_t clientId,
                       const Message &msg,
                       GameWorld &world,
                       Monitor &monitor);

private:
    GameFormulas formulas;

    void sendInventory(uint32_t clientId, Player &player, Monitor &monitor);
    void sendStats(uint32_t clientId, Player &player, Monitor &monitor);
};
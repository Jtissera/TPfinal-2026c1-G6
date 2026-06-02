#pragma once

#include "common/network/protocol/registry.h"
#include "common/network/messages/server/player/EntityMoveMessage.h"
#include "common/network/messages/server/player/playerStatsMessage.h"
#include "common/network/messages/server/player/playerDiedMessage.h"
#include "common/network/messages/server/player/levelUpMessage.h"
#include "common/network/messages/server/inventory/inventoryUpdateMessage.h"
#include "common/network/messages/server/inventory/itemOnGroundMessage.h"
#include "common/network/messages/server/inventory/itemPickedMessage.h"
#include "common/network/messages/server/combat/combatLogMessage.h"
#include "common/network/messages/server/npc/npcHealthMessage.h"
#include "common/network/messages/server/world/entitySpawnMessage.h"
#include "common/network/messages/server/world/entityDespawnMessage.h"
#include "common/network/messages/server/npc/npcListMessage.h"
#include "common/npcType.h"

#include "common/network/protocol/serverOpCode.h"
#include "common/dtos/itemDto.h"

class GameServerDeserializersModule {
public:
    void registerDeserializers(Registry& registry) const;
};
#include "ActionDispatcher.h"

#include "common/network/messages/client/inventory/pickItemMessage.h"
#include "common/network/messages/client/inventory/unequipSlotMessage.h"
#include "common/network/messages/client/inventory/useItemMessage.h"
#include "common/network/messages/server/inventory/goldOnGroundMessage.h"
#include "common/network/messages/server/inventory/itemOnGroundMessage.h"
#include "common/network/messages/server/inventory/itemPickedMessage.h"
#include "common/network/messages/server/npc/npcHealthMessage.h"
#include "common/network/messages/server/player/playerEquipmentUpdateMessage.h"
#include "common/network/messages/server/player/playerResurrectedMessage.h"
#include "server/game/equipmentDtoFactory.h"
#include "common/network/messages/server/chat/chatNotificationMessage.h"
#include "server/game/chat/chatHandler.h"
#include "server/game/clan/clanManager.h"
#include "common/network/messages/internal/clanSyncMessage.h"

static void sendCombatChat(uint32_t clientId,
                           const std::string &text,
                           ChatMsgType type,
                           Monitor &monitor)
{
    monitor.sendTo(clientId,
                   std::make_shared<const ChatNotificationMessage>(text, type));
}

ActionDispatcher::ActionDispatcher(const toml::table &config, ClanManager &clanManager)
    : combat(config),
      formulas(config),
      combatHandler(combat, effects, formulas),
      chatHandler(clanManager),
      clanManager(clanManager)
{
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_MOVE)] = &ActionDispatcher::handleMove;

    handlers[static_cast<uint8_t>(ClientOpCode::MSG_PICK_ITEM)] = &ActionDispatcher::handlePickItem;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_DROP_ITEM)] = &ActionDispatcher::handleDropItem;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_EQUIP_ITEM)] = &ActionDispatcher::handleEquipItem;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_UNEQUIP_SLOT)] = &ActionDispatcher::handleUnequipSlot;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_USE_ITEM)] = &ActionDispatcher::handleUseItem;

    handlers[static_cast<uint8_t>(ClientOpCode::MSG_MEDITATE)] = &ActionDispatcher::handleMeditate;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_RESURRECT)] = &ActionDispatcher::handleResurrect;

    handlers[static_cast<uint8_t>(ClientOpCode::MSG_ATTACK)] = &ActionDispatcher::handleAttack;

    handlers[static_cast<uint8_t>(ClientOpCode::MSG_CHEAT)] = &ActionDispatcher::handleCheat;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_INTERACT_NPC)] = &ActionDispatcher::handleInteractNpc;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_CHAT)] = &ActionDispatcher::handleChat;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_CLAN_SYNC_INTERNAL)] = &ActionDispatcher::handleClanSync;
}

void ActionDispatcher::dispatch(const ClientMessage &msg, GameWorld &world,
                                Monitor &monitor)
{
    uint8_t opcode = msg.message->opCode();

    if (!world.canPlayerAct(msg.clientId))
    {
        // Fantasmas solo pueden moverse, resucitar e interactuar con NPC
        // (para el comando /resucitar remoto)
        if (opcode != static_cast<uint8_t>(ClientOpCode::MSG_MOVE) &&
            opcode != static_cast<uint8_t>(ClientOpCode::MSG_RESURRECT) &&
            opcode != static_cast<uint8_t>(ClientOpCode::MSG_INTERACT_NPC) &&
            opcode != static_cast<uint8_t>(ClientOpCode::MSG_CHAT) &&
            opcode != static_cast<uint8_t>(ClientOpCode::MSG_CLAN_SYNC_INTERNAL))
            return;
    }

    auto it = handlers.find(opcode);
    if (it != handlers.end())
        (this->*it->second)(msg.clientId, *msg.message, world, monitor);
    else
        std::cerr << "[Dispatcher] OpCode no manejado: 0x" << std::hex
                  << (int)opcode << std::dec << std::endl;
}

void ActionDispatcher::sendStats(uint32_t id, Player &p, Monitor &monitor)
{
    monitor.sendTo(id, std::make_shared<const PlayerStatsMessage>(
                           p.getLevel(), p.getHp(), p.getMaxHp(), p.getMana(),
                           p.getMaxMana(), p.getExp(),
                           formulas.calcExpLimit(p.getLevel()), p.getGold()));
}

void ActionDispatcher::sendInventory(uint32_t id, Player &p, Monitor &monitor)
{
    monitor.sendTo(id, std::make_shared<const InventoryUpdateMessage>(
                           p.getInventory().getItems(),
                           p.getInventory().getInventorySlots(),
                           p.getInventory().getEquippedArray()));
}

void ActionDispatcher::sendDeath(uint32_t id, Player &dead, Monitor &monitor)
{
    sendInventory(id, dead, monitor);
    sendStats(id, dead, monitor);
    monitor.broadcast(std::make_shared<const PlayerDiedMessage>(id));
    monitor.sendTo(id, std::make_shared<const PlayerDiedMessage>(id));
}

void ActionDispatcher::handleMove(uint32_t id, const Message &msg, GameWorld &world, Monitor &monitor)
{

    std::cout << "[DEBUG] ActionDispatcher::handleMove - ID original: " << id << std::endl;
    const auto &moveMsg = static_cast<const MoveMessage &>(msg);
    const Direction direction = moveMsg.getDirection();
    const bool moving = moveMsg.isMoving();

    if (!moving)
    {
        monitor.broadcast(std::make_shared<const EntityMoveMessage>(
            static_cast<uint32_t>(id),
            world.getPixelX(id),
            world.getPixelY(id),
            direction,
            false));

        return;
    }

    if (world.movePlayer(id, direction))
    {
        monitor.broadcast(std::make_shared<const EntityMoveMessage>(
            static_cast<uint32_t>(id),
            world.getPixelX(id),
            world.getPixelY(id),
            direction,
            true));
    }
}

void ActionDispatcher::handlePickItem(uint32_t id, const Message& msg,GameWorld& world, Monitor& monitor) {
    Player& p = world.getPlayer(id);

    const auto& pickMsg = static_cast<const PickItemMessage&>(msg);

    if (pickMsg.getIsGold()) {
        auto gold = world.pickGoldById(pickMsg.getInstanceId());
        if (gold) {
            p.addGold(*gold);
            sendStats(id, p, monitor);
            monitor.broadcast(std::make_shared<const ItemPickedMessage>(id,pickMsg.getInstanceId()));

        }
        return;
    }

    if (!p.getInventory().canAddItem()) {
        return;
    }
    auto item = world.pickItemById(pickMsg.getInstanceId());
    if (item && p.getInventory().addItem(std::move(*item))) {
        sendInventory(id, p, monitor);
        monitor.broadcast(std::make_shared<const ItemPickedMessage>(id,pickMsg.getInstanceId()));
    }
}

void ActionDispatcher::handleMeditate(uint32_t id, const Message &msg, GameWorld &world, Monitor &monitor)
{
    Player &p = world.getPlayer(id);
    if (p.isMeditating())
        p.stopMeditating();
    else
        p.startMeditating();
    sendStats(id, p, monitor);
}

void ActionDispatcher::handleResurrect(uint32_t id, const Message &msg, GameWorld &world, Monitor &monitor)
{
    // El mensaje de resurrect no trae datos adicionales.
    (void)msg;

    std::cout << "[SERVER RESURRECT] client=" << id << std::endl;

    Player &p = world.getPlayer(id);

    std::cout << "[SERVER RESURRECT] before isGhost="
              << p.isGhost()
              << " hp="
              << p.getHp()
              << std::endl;

    // Si no está muerto/fantasma, no hacemos nada.
    if (!p.isGhost())
    {
        std::cout << "[SERVER RESURRECT] ignorado: player no es ghost id="
                  << id
                  << std::endl;
        return;
    }

    // Cheat de revive: tile fijo.
    const uint16_t reviveTileX = static_cast<uint16_t>(p.getTileX());
    const uint16_t reviveTileY = static_cast<uint16_t>(p.getTileY());

    world.resurrectPlayer(id, reviveTileX, reviveTileY);

    std::cout << "[SERVER RESURRECT] after isGhost="
              << p.isGhost()
              << " hp="
              << p.getHp()
              << " pos=("
              << world.getPixelX(id)
              << ", "
              << world.getPixelY(id)
              << ")"
              << std::endl;

    monitor.broadcast(
        std::make_shared<const PlayerResurrectedMessage>(
            id,
            reviveTileX,
            reviveTileY));

    // Avisamos a TODOS la posición actual.
    // Antes era sendTo(id), pero los remotos también tienen que moverlo.
    monitor.broadcast(
        std::make_shared<const EntityMoveMessage>(
            static_cast<uint32_t>(id),
            world.getPixelX(id),
            world.getPixelY(id),
            Direction::DOWN,
            false));

    // Stats solo al dueño para actualizar HUD.
    sendStats(id, p, monitor);

    std::cout << "[SERVER RESURRECT] broadcast resurrect id="
              << id
              << " tile=("
              << reviveTileX
              << ", "
              << reviveTileY
              << ")"
              << std::endl;
}

void ActionDispatcher::handleEquipItem(uint32_t id, const Message &msg, GameWorld &world, Monitor &monitor)
{
    const auto &equipMsg = static_cast<const EquipItemMessage &>(msg);
    const uint32_t itemInstanceId = equipMsg.getItemInstanceId();

    std::cout << "[SERVER EQUIP] client="
              << id
              << " itemInstanceId="
              << itemInstanceId
              << std::endl;

    if (!world.hasPlayer(id))
    {
        std::cerr << "[SERVER EQUIP] jugador inexistente id="
                  << id
                  << std::endl;
        return;
    }

    Player &player = world.getPlayer(id);

    const bool equipped = player.getInventory().equipItem(itemInstanceId);

    if (!equipped)
    {
        std::cerr << "[SERVER EQUIP] no se pudo equipar itemInstanceId="
                  << itemInstanceId
                  << std::endl;

        sendInventory(id, player, monitor);
        return;
    }

    std::cout << "[SERVER EQUIP] equipado correctamente itemInstanceId="
              << itemInstanceId
              << std::endl;

    sendInventory(id, player, monitor);
    std::cout << "[SERVER EQUIP UPDATE] broadcast playerId=" << id << std::endl;
    monitor.broadcast(std::make_shared<const PlayerEquipmentUpdateMessage>(id, buildEquipmentDtoFromPlayer(player)));
}

void ActionDispatcher::handleUnequipSlot(uint32_t id, const Message &msg, GameWorld &world, Monitor &monitor)
{
    const auto &unequipMsg = static_cast<const UnequipSlotMessage &>(msg);
    const EquipSlot slot = unequipMsg.getSlot();

    if (!world.hasPlayer(id))
    {
        return;
    }

    Player &player = world.getPlayer(id);

    const bool ok = player.getInventory().unequipSlot(slot);

    std::cout << "[SERVER UNEQUIP] client="
              << id
              << " slot="
              << static_cast<int>(slot)
              << " ok="
              << ok
              << std::endl;

    sendInventory(id, player, monitor);
    std::cout << "[SERVER EQUIP UPDATE] broadcast playerId="
              << id
              << std::endl;
    monitor.broadcast(std::make_shared<const PlayerEquipmentUpdateMessage>(id, buildEquipmentDtoFromPlayer(player)));
}

void ActionDispatcher::handleUseItem(
    uint32_t id,
    const Message &msg,
    GameWorld &world,
    Monitor &monitor)
{
    const auto &useMsg = static_cast<const UseItemMessage &>(msg);
    const uint32_t itemInstanceId = useMsg.getItemInstanceId();

    if (!world.hasPlayer(id))
    {
        std::cerr << "[SERVER USE ITEM] jugador inexistente id="
                  << id
                  << std::endl;
        return;
    }

    Player &player = world.getPlayer(id);

    const Item *item = player.getInventory().findItem(itemInstanceId);

    if (item == nullptr)
    {
        std::cerr << "[SERVER USE ITEM] item inexistente instanceId="
                  << itemInstanceId
                  << std::endl;

        sendInventory(id, player, monitor);
        return;
    }

    bool used = false;

    if (item->slot == ItemSlot::CONSUMABLE)
    {
        if (item->stats.healAmount > 0)
        {
            player.heal(item->stats.healAmount);
            used = true;
        }

        if (item->stats.manaAmount > 0)
        {
            player.restoreMana(item->stats.manaAmount);
            used = true;
        }
    }

    if (!used)
    {
        std::cerr << "[SERVER USE ITEM] item no consumible o sin efecto. instanceId="
                  << itemInstanceId
                  << std::endl;

        sendInventory(id, player, monitor);
        sendStats(id, player, monitor);
        return;
    }

    player.getInventory().removeItem(itemInstanceId);

    std::cout << "[SERVER USE ITEM] client="
              << id
              << " itemInstanceId="
              << itemInstanceId
              << " usado correctamente"
              << std::endl;

    sendStats(id, player, monitor);
    sendInventory(id, player, monitor);
}

void ActionDispatcher::handleAttack(uint32_t id, const Message &msg, GameWorld &world, Monitor &monitor)
{

    const auto &attackMsg = static_cast<const AttackMessage &>(msg);
    const uint32_t targetId = attackMsg.getTargetId();

    std::cout << "[SERVER ATTACK] attackerId="
              << id
              << " targetId="
              << targetId
              << std::endl;

    if (!world.hasPlayer(id))
    {
        std::cout << "[SERVER ATTACK] attacker inexistente id="
                  << id
                  << std::endl;
        return;
    }

    Player &attacker = world.getPlayer(id);

    if (!attacker.isAlive() || attacker.isGhost())
    {
        std::cout << "[SERVER ATTACK] attacker muerto/fantasma id="
                  << id
                  << std::endl;
        return;
    }

    if (world.hasPlayer(targetId))
    {
        std::cout << "[SERVER ATTACK] target es PLAYER id="
                  << targetId
                  << std::endl;
        handleAttackPlayer(id, targetId, world, monitor);
        return;
    }

    if (world.hasNpc(targetId))
    {
        std::cout << "[SERVER ATTACK] target es NPC id="
                  << targetId
                  << std::endl;
        handleAttackNpc(id, targetId, world, monitor);
        return;
    }

    std::cout << "[SERVER ATTACK] target inexistente targetId=" << targetId << " attackerId=" << id << std::endl;
}

void ActionDispatcher::handleAttackPlayer(uint32_t attackerId,uint32_t targetId,GameWorld &world,Monitor &monitor){

    if (attackerId == targetId)
    {
        return;
    }

    Player &attacker = world.getPlayer(attackerId);
    Player &target = world.getPlayer(targetId);

    // El atacante muerto/fantasma no puede atacar.
    if (!attacker.isAlive() || attacker.isGhost())
    {
        return;
    }

    // El target muerto/fantasma no puede ser atacado.
    if (!target.isAlive() || target.isGhost())
    {
        return;
    }

    const Item *weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);

    // Si el arma equipada es de curación, no procesamos como daño PvP.
    if (weapon != nullptr && weapon->effect == ItemEffect::HEAL)
    {
        if (effects.apply(*weapon, attacker, &target))
        {
            sendStats(attackerId, attacker, monitor);
            sendStats(targetId, target, monitor);
        }

        return;
    }

    auto result = combat.attackPlayer(attacker, target, world);

    if (!result.valid)
    {
        if (result.failReason == CombatSystem::Result::FailReason::FRIENDLY_FIRE)
        {
            sendCombatChat(attackerId,
                           "No podés atacar a tus aliados.",
                           ChatMsgType::INFO, monitor);
        }
        else if (result.failReason == CombatSystem::Result::FailReason::NO_MANA)
        {
            sendCombatChat(attackerId,"No tenés mana suficiente para realizar ese hechizo.",ChatMsgType::INFO, monitor);
        }
        else if (result.failReason == CombatSystem::Result::FailReason::LEVEL_TOO_LOW)
        {
            std::cout << "[COMBAT] attackerId=" << attackerId
          << " level=" << attacker.getLevel()
          << " targetId=" << targetId  
          << " level=" << target.getLevel()
          << " failReason=" << static_cast<int>(result.failReason)
          << std::endl;

            sendCombatChat(attackerId,
                           "No podés atacar a jugadores de nivel bajo.",
                           ChatMsgType::INFO, monitor);
        }
        else if (result.failReason == CombatSystem::Result::FailReason::LEVEL_DIFF_TOO_HIGH)
        {
            std::cout << "[COMBAT] attackerId=" << attackerId
          << " level=" << attacker.getLevel()
          << " targetId=" << targetId  
          << " level=" << target.getLevel()
          << " failReason=" << static_cast<int>(result.failReason)
          << std::endl;
          
            sendCombatChat(attackerId,
                           "La diferencia de nivel es demasiado grande para atacar.",
                           ChatMsgType::INFO, monitor);
        }
        sendStats(attackerId, attacker, monitor);
        return;
    }

    auto targetClanInfo = clanManager.findClanInfoForMember(target.getName());
    if (targetClanInfo)
    {
        std::string clanName = targetClanInfo->first;
        std::string alertMsg = "¡Nuestro aliado " + target.getName() + " está siendo atacado!";

        std::vector<uint32_t> localAllies = world.getOnlineClanMemberIds(clanName);

        for (uint32_t allyId : localAllies)
        {
            if (allyId == targetId)
                continue;
            sendCombatChat(allyId, alertMsg, ChatMsgType::CLAN, monitor);
        }
    }

    if (result.valid && !result.dodged)
    {
        monitor.sendTo(attackerId,std::make_shared<const CombatLogMessage>(std::to_string(targetId)));

        broadcastPlayerAttackVisual(attackerId, targetId, attacker, monitor);
    }
    // Si esquivó, actualizamos stats y terminamos.
    if (result.dodged)
    {
        sendCombatChat(attackerId,
                       "¡Tu ataque fue esquivado!",
                       ChatMsgType::INFO, monitor);
        sendCombatChat(targetId,
                       "¡Esquivaste el ataque!",
                       ChatMsgType::INFO, monitor);
        sendStats(attackerId, attacker, monitor);
        sendStats(targetId, target, monitor);
        return;
    }

    if (result.killed || target.getHp() <= 0) {
        GameWorld::DeathResult deathResult = world.handlePlayerDeath(targetId, attackerId);

        if (deathResult.excessGold > 0) {
            monitor.broadcast(std::make_shared<const GoldOnGroundMessage>(
                deathResult.goldInstanceId,
                deathResult.excessGold,
                deathResult.tileX,
                deathResult.tileY));
        }
        sendCombatChat(attackerId,
               "¡Mataste a " + target.getName() + "!",
               ChatMsgType::DAMAGE_DEALT, monitor);
        sendCombatChat(targetId,
                       "¡Fuiste asesinado por " + attacker.getName() + "!",
                       ChatMsgType::DAMAGE_TAKEN, monitor);

        for (const Item& item : deathResult.droppedItems) {
            monitor.broadcast(std::make_shared<const ItemOnGroundMessage>(
                item,
                deathResult.tileX,
                deathResult.tileY));
        }

        sendStats(attackerId, attacker, monitor);
        sendLevelUpIfNeeded(attackerId, attacker, monitor);
        sendInventory(targetId, target, monitor);
        sendDeath(targetId, target, monitor);
        monitor.broadcast(std::make_shared<const PlayerHealthMessage>(targetId,
            static_cast<uint16_t>(target.getHp()),
            static_cast<uint16_t>(target.getMaxHp())));

        return;
    }
    //  Caso normal: golpe válido, no esquivado, no mató.

    sendCombatChat(attackerId,
                   "Causaste " + std::to_string(result.damage) + " pts de daño a " + target.getName() + ".",
                   ChatMsgType::DAMAGE_DEALT, monitor);

    sendCombatChat(targetId,
                   attacker.getName() + " te causó " + std::to_string(result.damage) + " pts de daño.",
                   ChatMsgType::DAMAGE_TAKEN, monitor);

    world.giveExperience(attackerId, result.expGained);

    sendStats(attackerId, attacker, monitor);
    sendStats(targetId, target, monitor);
    sendLevelUpIfNeeded(attackerId, attacker, monitor);
    monitor.broadcast(std::make_shared<const PlayerHealthMessage>(targetId,target.getHp(),target.getMaxHp()));
}
void ActionDispatcher::handleAttackNpc(uint32_t attackerId,uint32_t npcId,GameWorld &world,Monitor &monitor){
    Player &attacker = world.getPlayer(attackerId);

    // Jugador muerto/fantasma no puede atacar.
    if (!attacker.isAlive() || attacker.isGhost())
    {
        return;
    }

    const Item *weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);

    // Hechizos/ítems de curación no se procesan como ataque PvE.
    if (weapon != nullptr && weapon->effect == ItemEffect::HEAL)
    {
        return;
    }

    Npc &npc = world.getNpc(npcId);

    // NPC muerto o en respawn no puede recibir ataque.
    if (!npc.isAlive())
    {
        return;
    }

    // NPC no hostil no recibe ataque PvE.
    if (!npc.isHostile())
    {
        return;
    }

    auto result = combat.attackNpc(attacker, npc, world);

    // Si el ataque no fue válido, reenviamos stats por si se consumió algo antes.
    if (!result.valid)
    {
        if (result.failReason == CombatSystem::Result::FailReason::NO_MANA)
        {
            sendCombatChat(attackerId,
                           "No tenés mana suficiente para realizar ese hechizo.",
                           ChatMsgType::INFO, monitor);
        }
        sendStats(attackerId, attacker, monitor);
        return;
    }

    if (!result.dodged)
    {
        monitor.sendTo(attackerId,std::make_shared<const CombatLogMessage>(std::to_string(npcId)));
        broadcastPlayerAttackVisual(attackerId,npcId,attacker,monitor);
    }

    // Siempre informamos vida nueva del NPC después del ataque válido.
    monitor.broadcast(
        std::make_shared<const NpcHealthMessage>(
            npcId,
            npc.getHp(),
            npc.getMaxHp()));

    // Si el NPC murió, procesamos muerte, oro directo y respawn.
    if (result.killed) {
        NpcDropResult dropResult = world.handleNpcDeath(npcId, attackerId);

        if (dropResult.hasGold) {
            monitor.broadcast(std::make_shared<const GoldOnGroundMessage>(
                dropResult.goldInstanceId,
                dropResult.goldAmount,
                dropResult.tileX,
                dropResult.tileY));
        }


        if (dropResult.hasItem) {
            monitor.broadcast(std::make_shared<const ItemOnGroundMessage>(
                dropResult.droppedItem,
                dropResult.tileX,
                dropResult.tileY));
        }
        sendCombatChat(attackerId,
                "¡Mataste al " + npc.getName() + "!",
                ChatMsgType::DAMAGE_DEALT, monitor);
        sendStats(attackerId, attacker, monitor);
        sendLevelUpIfNeeded(attackerId, attacker, monitor);

        return;
    }

    // Si esquivó, no damos experiencia ni cambiamos target.
    if (result.dodged)
    {
        sendCombatChat(attackerId,
                       "¡El " + npc.getName() + " esquivó tu ataque!",
                       ChatMsgType::INFO, monitor);
        sendStats(attackerId, attacker, monitor);
        return;
    }

    sendCombatChat(attackerId,
                   "Causaste " + std::to_string(result.damage) + " pts de daño al " + npc.getName() + ".",
                   ChatMsgType::DAMAGE_DEALT, monitor);

    // Experiencia por daño.
    world.giveExperience(attackerId, result.expGained);

    // El NPC empieza a perseguir al atacante.
    npc.setTargetId(attackerId);
    npc.setState(NpcState::CHASING);

    // Actualizamos stats y posible level up.
    sendStats(attackerId, attacker, monitor);
    sendLevelUpIfNeeded(attackerId, attacker, monitor);
}

void ActionDispatcher::sendLevelUpIfNeeded(uint32_t playerId, Player &player, Monitor &monitor)
{

    if (!player.checkAndClearLevelUp())
    {
        return;
    }

    monitor.broadcast(std::make_shared<const LevelUpMessage>(playerId, static_cast<uint8_t>(player.getLevel())));
}

void ActionDispatcher::handleDropItem(uint32_t id, const Message &msg,
                                      GameWorld &world, Monitor &monitor)
{
    const auto &dropMsg = static_cast<const DropItemMessage &>(msg);
    Player &p = world.getPlayer(id);

    auto removed = p.getInventory().removeItem(dropMsg.getItemId());
    if (removed)
    {
        world.addItemOnGround(std::move(*removed), p.getTileX(), p.getTileY());
        sendInventory(id, p, monitor);
    }
}

void ActionDispatcher::handleCheat(uint32_t id, const Message &msg,
                                   GameWorld &world, Monitor &monitor)
{
  std::cout << "[CHEAT DEBUG] handleCheat invocado clientId=" << id << std::endl;
  const auto &cheatMsg = static_cast<const CheatMessage &>(msg);
  std::cout << "[CHEAT DEBUG] tipo=" << static_cast<int>(cheatMsg.getCheat()) << std::endl;
  Player &p = world.getPlayer(id);
  switch (cheatMsg.getCheat())
  {
  case CheatType::INFINITE_HP:
    std::cout << "[CHEAT DEBUG] INFINITE_HP inicio" << std::endl;
    p.toggleInfiniteHp();
    std::cout << "[CHEAT DEBUG] INFINITE_HP toggle hecho" << std::endl;
    sendStats(id, p, monitor);
    std::cout << "[CHEAT DEBUG] INFINITE_HP sendStats OK" << std::endl;
    break;
  case CheatType::INFINITE_MANA:
    std::cout << "[CHEAT DEBUG] INFINITE_MANA inicio" << std::endl;
    p.toggleInfiniteMana();
    sendStats(id, p, monitor);
    std::cout << "[CHEAT DEBUG] INFINITE_MANA fin" << std::endl;
    break;
  case CheatType::DIE:
      std::cout << "[CHEAT DEBUG] DIE inicio" << std::endl;
      if (!p.isAlive() || p.isGhost()) {
          std::cout << "[CHEAT DEBUG] DIE early return" << std::endl;
          return;
      }
      {
      GameWorld::DeathResult deathResult = world.handlePlayerDeath(id, 0);
      std::cout << "[CHEAT DEBUG] DIE handlePlayerDeath OK excessGold=" << deathResult.excessGold << std::endl;
      if (deathResult.excessGold > 0) {
          monitor.broadcast(std::make_shared<const GoldOnGroundMessage>(
              deathResult.goldInstanceId,
              deathResult.excessGold,
              deathResult.tileX,
              deathResult.tileY));
          std::cout << "[CHEAT DEBUG] DIE gold broadcast OK" << std::endl;
      }
      for (const Item& item : deathResult.droppedItems) {
          monitor.broadcast(std::make_shared<const ItemOnGroundMessage>(item,deathResult.tileX,deathResult.tileY));
      }
      std::cout << "[CHEAT DEBUG] DIE items broadcast OK" << std::endl;
      }
      sendDeath(id, p, monitor);
      std::cout << "[CHEAT DEBUG] DIE sendDeath OK" << std::endl;
      break;
  case CheatType::ADD_GOLD:
          std::cout << "[CHEAT DEBUG] ADD_GOLD inicio" << std::endl;
          p.addGold(1000);
          sendInventory(id,p,monitor);
          std::cout << "[CHEAT DEBUG] ADD_GOLD sendInventory OK" << std::endl;
          sendStats(id,p,monitor);
          std::cout << "[CHEAT DEBUG] ADD_GOLD sendStats OK" << std::endl;
      break;
  case CheatType::LEVEL_UP:
          std::cout << "[CHEAT DEBUG] LEVEL_UP inicio" << std::endl;
          world.giveExperience(id,100000);
          sendStats(id,p,monitor);
          sendLevelUpIfNeeded(id,p,monitor);
          std::cout << "[CHEAT DEBUG] LEVEL_UP fin" << std::endl;
      break;
  }
  std::cout << "[CHEAT DEBUG] handleCheat fin" << std::endl;
}

void ActionDispatcher::handleInteractNpc(uint32_t id, const Message &msg,
                                         GameWorld &world, Monitor &monitor)
{
    const auto &interactMsg = static_cast<const InteractNpcMessage &>(msg);
    Player &player = world.getPlayer(id);

    if (player.isGhost())
    {
        auto cmd = CityCommandParser::parse(interactMsg.getCmd());
        if (!cmd || cmd->type != CityCommand::Type::RESURRECT)
        {
            monitor.sendTo(id, std::make_shared<const ErrorMessage>(
                                   "Un fantasma no puede interactuar."));
            return;
        }
        auto result = world.handleRemoteResurrect(id);
        monitor.sendTo(id, std::make_shared<const NpcResponseMessage>(result.message));

        return;
    }

    // Buscar NPC de ciudad en tiles adyacentes al jugador (distancia Chebyshev <= 1)
    int px = player.getTileX();
    int py = player.getTileY();

    std::optional<NpcType> npcType;
    for (int dx = -1; dx <= 1 && !npcType; dx++)
        for (int dy = -1; dy <= 1 && !npcType; dy++)
            npcType = world.getNpcTypeAtTile(px + dx, py + dy);

    if (!npcType)
    {
        monitor.sendTo(id, std::make_shared<const ErrorMessage>(
                               "No hay ningún NPC cerca."));
        return;
    }

    auto cmd = CityCommandParser::parse(interactMsg.getCmd());
    if (!cmd)
    {
        monitor.sendTo(id, std::make_shared<const ErrorMessage>("Comando inválido."));
        return;
    }

    auto result = world.handleCityInteraction(id, *npcType, *cmd);
    monitor.sendTo(id, std::make_shared<const NpcResponseMessage>(result.message));
    if (result.ok)
        sendStats(id, player, monitor);
}

void ActionDispatcher::handleChat(uint32_t id,
                                  const Message &msg,
                                  GameWorld &world,
                                  Monitor &monitor)
{
    const auto &chatMsg = static_cast<const ChatMessage &>(msg);
    chatHandler.handle(id,
                       chatMsg.getText(),
                       chatMsg.getTargetId(),
                       world,
                       monitor);
}

void ActionDispatcher::handleClanSync(uint32_t id, const Message &msg,
                                      GameWorld &world, Monitor &monitor)
{
    (void)monitor;

    if (!world.hasPlayer(id))
        return;

    const auto &syncMsg = static_cast<const ClanSyncMessage &>(msg);
    Player &player = world.getPlayer(id);
    player.setClanName(syncMsg.getClanName());
    player.setClanFounder(syncMsg.getIsFounder());
}

PlayerAttackVisualType ActionDispatcher::resolveAttackVisualType(
    const Player& attacker) const
{
    // Buscamos el arma equipada en la mano.
    const Item* weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);

    // Sin arma: ataque físico básico.
    if (weapon == nullptr)
    {
        return PlayerAttackVisualType::Physical;
    }

    // Bastones/varas: ataque mágico.
    if (weapon->slot == ItemSlot::STAFF)
    {
        return PlayerAttackVisualType::Magic;
    }

    // Arcos u otras armas marcadas como rango desde TOML.
    if (weapon->stats.isRanged)
    {
        return PlayerAttackVisualType::Ranged;
    }

    // Espadas, hachas, martillos, etc.
    return PlayerAttackVisualType::Physical;
}

void ActionDispatcher::broadcastPlayerAttackVisual(uint32_t attackerId,
                                                   uint32_t targetId,
                                                   const Player& attacker,
                                                   Monitor& monitor)
{
    const PlayerAttackVisualType visualType =
        resolveAttackVisualType(attacker);

    monitor.broadcast(std::make_shared<const PlayerAttackVisualMessage>(
        attackerId,
        targetId,
        visualType));
}
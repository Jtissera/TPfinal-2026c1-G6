#include "gameRoom.h"

GameRoom::GameRoom(
    uint32_t gameId, std::string gameName, const std::string &mapPath,
    bool isInstance, uint32_t originRoomId, NpcFactory &npcFactory,
    ItemRepository &itemRepo,
    Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
    Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue,
    const toml::table &config, PlayerArchive &archive,
    ClanManager &clanManager)
    : config(config),
      formulas(config), gameId(gameId), gameName(std::move(gameName)), maxPlayers(255),
      isInstance(isInstance), originRoomId(originRoomId), mapPath(mapPath), leaveQueue(leaveQueue),
      archive(archive), clanManager(clanManager), world(mapPath, npcFactory, itemRepo, config, clanManager),
      gameLoop(gameQueue, monitor, world, leaveQueue, transitionQueue,
               gameId, config, archive, mapPath, clanManager, originRoomId)
{
}

bool GameRoom::getIsInstance() const { return isInstance; }
uint32_t GameRoom::getOriginRoomId() const { return originRoomId; }
const std::string &GameRoom::getMapPath() const { return mapPath; }

void GameRoom::addClient(uint32_t clientId,
                         Queue<std::shared_ptr<const Message>> &clientQueue)
{
  monitor.addQueue(clientId, clientQueue);
}

void GameRoom::addPlayer(Player player)
{
  world.addPlayer(std::move(player));
}

void GameRoom::removeClient(uint32_t clientId)
{
  std::shared_ptr<EntityDespawnMessage> despawnMsg =
      std::make_shared<EntityDespawnMessage>(clientId);
  broadcastExcept(clientId, despawnMsg);
  monitor.removeQueue(clientId);
  world.removePlayer(clientId);
}

Queue<ClientMessage> &GameRoom::getGameQueue() { return gameQueue; }
uint32_t GameRoom::getId() const { return gameId; }
const std::string &GameRoom::getName() const { return gameName; }
uint8_t GameRoom::getPlayerCount() const { return monitor.size(); }
uint8_t GameRoom::getMaxPlayers() const { return maxPlayers; }
bool GameRoom::isFull() const { return monitor.size() >= maxPlayers; }
void GameRoom::start() { gameLoop.start(); }
void GameRoom::stop() { gameLoop.stop(); }
void GameRoom::join() { gameLoop.join(); }

void GameRoom::broadcastExcept(uint32_t excludeId,
                               const std::shared_ptr<const Message> &msg)
{
  monitor.broadcastExcept(excludeId, msg);
}

const GameWorld &GameRoom::getWorld() const { return world; }

void GameRoom::sendExistingPlayersTo(uint32_t newClientId)
{
  for (const std::pair<const uint32_t, Player> &entry :
       world.getPlayers())
  {
    const uint32_t playerId = entry.first;
    const Player &player = entry.second;

    if (playerId == newClientId)
    {
      continue;
    }

    PlayerDto dto = buildPlayerDto(player);
    monitor.sendTo(newClientId,
                   std::make_shared<const EntitySpawnMessage>(std::move(dto)));
    monitor.sendTo(newClientId,
                   std::make_shared<const PlayerEquipmentUpdateMessage>(
                       playerId, buildEquipmentDtoFromPlayer(player)));
  }
}

void GameRoom::broadcastPlayerSpawn(uint32_t playerId)
{
  const Player &player = world.getPlayer(playerId);
  PlayerDto dto = buildPlayerDto(player);
  monitor.broadcast(
      std::make_shared<const EntitySpawnMessage>(std::move(dto)));
  monitor.broadcast(std::make_shared<const PlayerEquipmentUpdateMessage>(
      playerId, buildEquipmentDtoFromPlayer(player)));
}

void GameRoom::syncPlayerJoin(uint32_t newPlayerId)
{
  if (!world.hasPlayer(newPlayerId))
  {
    std::cerr << "[GameRoom] syncPlayerJoin: unknown player id="
              << newPlayerId << std::endl;
    return;
  }

  sendInventoryTo(newPlayerId);
  sendExistingPlayersTo(newPlayerId);
  sendExistingNpcsTo(newPlayerId);
  sendExistingGroundItemsTo(newPlayerId);
  broadcastPlayerSpawn(newPlayerId);
  sendStatsTo(newPlayerId);
}

void GameRoom::sendInventoryTo(uint32_t playerId)
{
  Player &player = world.getPlayer(playerId);
  monitor.sendTo(playerId,
                 std::make_shared<const InventoryUpdateMessage>(
                     player.getInventory().getItems(),
                     player.getInventory().getInventorySlots(),
                     player.getInventory().getEquippedArray()));
}

void GameRoom::sendExistingNpcsTo(uint32_t clientId)
{
  for (const std::pair<const uint32_t, Npc> &entry : world.getNpcs())
  {
    const uint32_t npcId = entry.first;
    const Npc &npc = entry.second;
    monitor.sendTo(clientId,
                   std::make_shared<const NpcSpawnMessage>(
                       npcId, npc.getType(), npc.getName(),
                       static_cast<uint16_t>(npc.getTileX() * 96),
                       static_cast<uint16_t>(npc.getTileY() * 96),
                       npc.getHp(), npc.getMaxHp(),
                       npc.getLevel(), npc.isHostile()));
  }
}

void GameRoom::sendExistingGroundItemsTo(uint32_t clientId)
{
  const GroundManager &ground = world.getGroundManager();
  const std::vector<GroundItem> &items = ground.getAllItems();
  const std::vector<GroundGold> &gold = ground.getAllGold();

  for (const GroundItem &groundItem : items)
  {
    monitor.sendTo(clientId,
                   std::make_shared<const ItemOnGroundMessage>(
                       groundItem.item, groundItem.tileX, groundItem.tileY));
  }

  for (const GroundGold &groundGold : gold)
  {
    monitor.sendTo(clientId,
                   std::make_shared<const GoldOnGroundMessage>(
                       groundGold.instanceId, groundGold.amount,
                       groundGold.tileX, groundGold.tileY));
  }
}

const Player *GameRoom::findPlayer(uint32_t clientId) const
{
  if (!world.hasPlayer(clientId))
  {
    return nullptr;
  }
  return &world.getPlayer(clientId);
}

void GameRoom::sendTo(uint32_t clientId,
                      const std::shared_ptr<const Message> &msg)
{
  monitor.sendTo(clientId, msg);
}

void GameRoom::removeMonitorOnly(uint32_t clientId)
{
  monitor.removeQueue(clientId);
}

void GameRoom::sendStatsTo(uint32_t playerId)
{
  const Player &player = world.getPlayer(playerId);
  monitor.sendTo(playerId,
                 std::make_shared<const PlayerStatsMessage>(
                     player.getLevel(),
                     player.getHp(), player.getMaxHp(),
                     player.getMana(), player.getMaxMana(),
                     player.getExp(),
                     formulas.calcExpLimit(player.getLevel()),
                     player.getGold()));
}

PlayerDto GameRoom::buildPlayerDto(const Player &player) const
{
  const int16_t defaultExpMax =
      config["player"]["default_exp_max"].value_or<int16_t>(1000);
  const uint8_t defaultIntelligence =
      config["player"]["default_intelligence"].value_or<uint8_t>(10);
  const uint8_t defaultConstitution =
      config["player"]["default_constitution"].value_or<uint8_t>(10);

  PlayerDto dto{};
  dto.playerID = player.getClientId();
  dto.nombre = player.getName();
  dto.raza = player.getRace().name;
  dto.clase = player.getCls().name;
  dto.clanName = player.getClanName();
  dto.headId = 0;
  dto.level = player.getLevel();
  dto.hp = player.getHp();
  dto.hpMax = player.getMaxHp();
  dto.mana = player.getMana();
  dto.manaMax = player.getMaxMana();
  dto.oro = static_cast<int>(player.getGold());
  dto.oroMax = 0;
  dto.xpos = static_cast<uint16_t>(player.getPixelX());
  dto.ypos = static_cast<uint16_t>(player.getPixelY());
  dto.exp = static_cast<int>(player.getExp());
  dto.expMax = formulas.calcExpLimit(player.getLevel());
  dto.esFantasma = player.isGhost();
  dto.fuerza = player.getStrength();
  dto.agilidad = player.getAgility();
  dto.inteligencia = defaultIntelligence;
  dto.constitucion = defaultConstitution;
  return dto;
}
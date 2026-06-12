#include "gameRoom.h"

#include "common/network/messages/server/npc/npcSpawnMessage.h"

GameRoom::GameRoom(
    uint32_t gameId, std::string gameName, const std::string &mapPath,
    bool isInstance, uint32_t originRoomId, NpcFactory &npcFactory,
    ItemRepository &itemRepo, Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
    Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue,
    const toml::table &config)
    : gameId(gameId), gameName(std::move(gameName)), maxPlayers(255),
      isInstance(isInstance), originRoomId(originRoomId), mapPath(mapPath),
      leaveQueue(leaveQueue), world(mapPath, npcFactory, itemRepo, config),
      gameLoop(gameQueue, monitor, world, leaveQueue, transitionQueue, gameId,
               config) {}

void GameRoom::addClient(uint32_t clientId,
                         Queue<std::shared_ptr<const Message>> &clientQueue)
{
  monitor.addQueue(clientId, clientQueue);
}

void GameRoom::addPlayer(Player player) { world.addPlayer(std::move(player)); }

void GameRoom::removeClient(uint32_t clientId)
{
  auto despawnMsg = std::make_shared<EntityDespawnMessage>(clientId);

  broadcastExcept(clientId, despawnMsg);

  monitor.removeQueue(clientId);
  world.removePlayer(clientId);

  std::cout << "[SERVER] Broadcast despawn por desconexión de playerId=" << clientId << std::endl;
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
  for (const auto &[playerId, player] : world.getPlayers())
  {
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

    std::cout << "[GameRoom] enviado existing player=" << playerId
              << " a newClientId=" << newClientId << std::endl;
  }
}

void GameRoom::broadcastPlayerSpawn(uint32_t playerId)
{
  const Player &player = world.getPlayer(playerId);

  PlayerDto dto = buildPlayerDto(player);

  monitor.broadcast(std::make_shared<const EntitySpawnMessage>(std::move(dto)));

  monitor.broadcast(std::make_shared<const PlayerEquipmentUpdateMessage>(
      playerId, buildEquipmentDtoFromPlayer(player)));

  std::cout << "[GameRoom] broadcast spawn playerId=" << playerId << std::endl;
}

void GameRoom::syncPlayerJoin(uint32_t newPlayerId)
{
  std::cout << "[GameRoom] syncPlayerJoin newPlayerId=" << newPlayerId
            << std::endl;

  if (!world.hasPlayer(newPlayerId))
  {
    std::cerr << "[GameRoom] syncPlayerJoin jugador inexistente id="
              << newPlayerId << std::endl;
    return;
  }

  sendInventoryTo(newPlayerId);
  sendExistingPlayersTo(newPlayerId);
  sendExistingNpcsTo(newPlayerId);
  broadcastPlayerSpawn(newPlayerId);
}

void GameRoom::sendInventoryTo(uint32_t playerId)
{
  Player &player = world.getPlayer(playerId);

  monitor.sendTo(playerId, std::make_shared<const InventoryUpdateMessage>(
                               player.getInventory().getItems(),
                               player.getInventory().getInventorySlots(),
                               player.getInventory().getEquippedArray()));

  std::cout << "[GameRoom] inventario enviado a playerId=" << playerId
            << " items=" << player.getInventory().getItems().size()
            << std::endl;
}

PlayerDto GameRoom::buildPlayerDto(const Player &player) const
{
  PlayerDto dto{};

  // Identidad del jugador.
  dto.playerID = static_cast<uint8_t>(player.getClientId());

  // Nombre visible.
  dto.nombre = player.getName();

  // Raza y clase.
  dto.raza = player.getRace().name;
  dto.clase = player.getCls().name;

  // Apariencia.
  dto.headId = 0;

  // Progresión.
  dto.level = player.getLevel();

  // Vida y maná.
  dto.hp = player.getHp();
  dto.hpMax = player.getMaxHp();
  dto.mana = player.getMana();
  dto.manaMax = player.getMaxMana();

  // Oro.
  dto.oro = static_cast<int>(player.getGold());
  dto.oroMax = 0;

  // Posición en píxeles.
  // Importante: NO usar tile * 32.
  dto.xpos = static_cast<uint16_t>(player.getPixelX());
  dto.ypos = static_cast<uint16_t>(player.getPixelY());

  // Experiencia.
  dto.exp = static_cast<int>(player.getExp());
  dto.expMax = 1000;

  // Estado.
  dto.esFantasma = player.isGhost();

  // Atributos.
  dto.fuerza = player.getStrength();
  dto.agilidad = player.getAgility();
  dto.inteligencia = 10;
  dto.constitucion = 10;

  return dto;
}

void GameRoom::sendExistingNpcsTo(uint32_t clientId)
{
  std::cout << "[GameRoom] sendExistingNpcsTo clientId=" << clientId
            << " npcCount=" << world.getNpcs().size() << std::endl;

  for (const auto &[npcId, npc] : world.getNpcs())
  {
    monitor.sendTo(clientId, std::make_shared<const NpcSpawnMessage>(
                                 npcId, npc.getType(), npc.getName(),
                                 static_cast<uint16_t>(npc.getTileX() * 96),
                                 static_cast<uint16_t>(npc.getTileY() * 96),
                                 npc.getHp(), npc.getMaxHp(), npc.isHostile()));

    std::cout << "[GameRoom] enviado NPC id=" << npcId
              << " a clientId=" << clientId << std::endl;
  }
}
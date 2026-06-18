#include "gameLoop.h"

#include "common/network/messages/server/inventory/goldOnGroundMessage.h"
#include "common/network/messages/server/inventory/itemOnGroundMessage.h"
#include "common/network/messages/server/npc/npcSpawnMessage.h"
#include "common/network/messages/server/chat/chatNotificationMessage.h"
#include "common/network/messages/server/npc/npcMoveMessage.h"
#include "common/network/messages/server/npc/npcSpawnMessage.h"
#include "common/network/messages/server/player/playerResurrectedMessage.h"

GameLoop::GameLoop(Queue<ClientMessage> &q, Monitor &m, GameWorld &w,
                   Queue<std::shared_ptr<LeaveEvent>> &leaveQ,
                   Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQ,
                   uint32_t gameId, const toml::table &config,
                   PlayerArchive &archive, const std::string &mapId,
                   ClanManager &clanManager)
    : gameQueue(q), monitor(m), world(w), leaveQueue(leaveQ),
      transitionQueue(transitionQ), gameId(gameId), dispatcher(config, clanManager),
      statManager(config),
      tickRateMs(config["server"]["tick_rate_ms"].value_or(33)),
      archive(archive), mapId(mapId),
      persistEveryNTicks(
          config["server"]["persist_every_n_ticks"].value_or(300)) {}

void GameLoop::run()
{
  using Clock = std::chrono::steady_clock;
  using Ms = std::chrono::duration<float, std::milli>;
  using Duration = std::chrono::milliseconds;

  auto t1 = Clock::now();

  try
  {
    while (true)
    {
      ClientMessage incoming;
      while (gameQueue.try_pop(incoming))
        processMessage(incoming);

      auto t2 = Clock::now();
      float elapsed = std::chrono::duration_cast<Ms>(t2 - t1).count();
      float rest = tickRateMs - elapsed;

      if (rest < 0)
      {
        float behind = -rest;
        rest = tickRateMs - std::fmod(behind, tickRateMs);
        float lost = behind + rest;
        t1 += Duration(static_cast<long>(lost));
      }
      else
      {
        std::this_thread::sleep_for(Duration(static_cast<long>(rest)));
      }

      worldUpdate(tickRateMs / 1000.0f);
      t1 += Duration(static_cast<long>(tickRateMs));
    }
  }
  catch (const ClosedQueue &)
  {
  }
  catch (const std::exception &e)
  {
    std::cerr << "[GameLoop] error: " << e.what() << std::endl;
  }
}

void GameLoop::processMessage(const ClientMessage &incoming)
{
  if (incoming.message->opCode() ==
      static_cast<uint8_t>(ClientOpCode::MSG_LEAVE_GAME))
  {
    handleLeaveGame(incoming.clientId);
    return;
  }
  dispatcher.dispatch(incoming, world, monitor);
}

void GameLoop::worldUpdate(float deltaSeconds)
{

  auto result = world.tick(deltaSeconds);

  for (const auto &info : result.resurrectionStarted)
  {
    monitor.sendTo(info.playerId,
                   std::make_shared<ResurrectionStartedMessage>(info.delayMs));
    std::cout << "[GameLoop] Enviando MSG_RESURRECTION_STARTED a player "
              << info.playerId << " Delay: " << info.delayMs << "ms"
              << std::endl;
  }

  // Si cambiaron stats de jugadores, mandamos stats actualizadas.
  for (const auto &hit : result.playerHits)
  {
    statManager.sendPlayerStats(hit.playerId, world, monitor);

    // Notificación en el mini-chat del jugador golpeado.
    monitor.sendTo(hit.playerId,
                   std::make_shared<const ChatNotificationMessage>(
                       "Un enemigo te causó " + std::to_string(hit.damage) +
                           " puntos de daño.",
                       ChatMsgType::DAMAGE_TAKEN));
  }

  for (const auto &allyHit : result.clanAllyHits)
  {
    std::vector<uint32_t> allies = world.getOnlineClanMemberIds(allyHit.clanName);
    std::string msg = "¡Nuestro aliado " + allyHit.targetName + " está siendo atacado!";
    for (uint32_t allyId : allies)
    {
      if (allyId == allyHit.targetId)
        continue;
      monitor.sendTo(allyId,
                     std::make_shared<const ChatNotificationMessage>(
                         msg, ChatMsgType::CLAN));
    }
  }

  for (uint32_t id : result.playersChanged)
  {
    statManager.sendPlayerStats(id, world, monitor);
  }

  for (auto &[playerId, deathResult] : result.playerDeathsByNpc)
  {
    if (deathResult.excessGold > 0)
    {
      monitor.broadcast(std::make_shared<const GoldOnGroundMessage>(
          deathResult.goldInstanceId,
          deathResult.excessGold,
          deathResult.tileX,
          deathResult.tileY));
    }

    for (const Item &item : deathResult.droppedItems)
    {
      monitor.broadcast(std::make_shared<const ItemOnGroundMessage>(
          item,
          deathResult.tileX,
          deathResult.tileY));
    }

    Player &victim = world.getPlayer(playerId);
    monitor.sendTo(playerId, std::make_shared<const InventoryUpdateMessage>(
                                  victim.getInventory().getItems(),
                                  victim.getInventory().getInventorySlots(),
                                  victim.getInventory().getEquippedArray()));

    std::cout << "[GameLoop] inventario purgado enviado a victima playerId="
              << playerId << std::endl;
  }

  for (uint32_t deadPlayerId : result.playersDied)
  {
    monitor.broadcast(std::make_shared<const PlayerDiedMessage>(deadPlayerId));

  }

  for (const auto &res : result.playersResurrected)
  {
    monitor.broadcast(std::make_shared<const PlayerResurrectedMessage>(
        res.playerId, res.tileX, res.tileY));

    monitor.broadcast(std::make_shared<const EntityMoveMessage>(
        static_cast<uint8_t>(res.playerId), world.getPixelX(res.playerId),
        world.getPixelY(res.playerId), Direction::DOWN, false));

    std::cout << "[GameLoop] broadcast PLAYER_RESURRECTED id=" << res.playerId
              << " tile=(" << res.tileX << ", " << res.tileY << ")"
              << std::endl;
  }
  // Si algún NPC se movió, avisamos al cliente con un mensaje específico.
  // Esto NO crea NPCs. Solo actualiza su posición visual.
  for (uint32_t npcId : result.npcsMoved)
  {
    const Npc &npc = world.getNpc(npcId);

    monitor.broadcast(std::make_shared<const NpcMoveMessage>(
        npc.getId(), static_cast<uint16_t>(npc.getTileX() * 96),
        static_cast<uint16_t>(npc.getTileY() * 96)));
  }

  for (const auto &npcSpawn : result.spawnedNpcs)
  {
    monitor.broadcast(std::make_shared<const NpcSpawnMessage>(
        npcSpawn.npcId, npcSpawn.type, npcSpawn.name, npcSpawn.x, npcSpawn.y,
        npcSpawn.hp, npcSpawn.maxHp, npcSpawn.hostile));

    std::cout << "[GameLoop] broadcast NPC respawn id=" << npcSpawn.npcId
              << " name=" << npcSpawn.name << std::endl;
  }

  // Transiciones de instancia.
  for (auto &entry : result.instanceTransitions)
  {
    handleInstanceTransition(entry);
  }

  ++persistTickCounter;
  if (persistTickCounter >= persistEveryNTicks)
  {
    persistTickCounter = 0;
    for (const auto &[clientId, player] : world.getPlayers())
    {
      archive.enqueue(archive.toSnapshot(player, mapId, gameId), gameId);
    }
  }
}

void GameLoop::handleLeaveGame(uint32_t clientId)
{
  if (world.hasPlayer(clientId))
  {
    const Player &player = world.getPlayer(clientId);
    // archive.enqueue(archive.toSnapshot(player, mapId, gameId), gameId);
  }

  auto player = world.removePlayer(clientId);
  if (!player)
    return;

  Queue<std::shared_ptr<const Message>> *clientQueue =
      monitor.getQueue(clientId);
  monitor.removeQueue(clientId);

  leaveQueue.try_push(std::make_shared<LeaveEvent>(
      LeaveEvent{clientId, gameId, std::move(*player), clientQueue}));
}

void GameLoop::handleInstanceTransition(const GameWorld::InstanceEntry &entry)
{
  auto player = world.removePlayer(entry.playerId);
  if (!player)
    return;

  monitor.broadcastExcept(
      entry.playerId,
      std::make_shared<const EntityDespawnMessage>(entry.playerId));

  Queue<std::shared_ptr<const Message>> *clientQueue =
      monitor.getQueue(entry.playerId);

  if (entry.targetMap.empty())
  {
    monitor.removeQueue(entry.playerId);
    transitionQueue.try_push(std::make_shared<InstanceTransitionEvent>(
        InstanceTransitionEvent{entry.playerId, gameId, std::move(*player),
                                clientQueue, entry.targetMap, 3, 3}));
    return;
  }

  std::string resolvedMapPath = entry.targetMap;
  if (resolvedMapPath.find("assets/") == std::string::npos)
  {
    std::string baseDir = "assets/sprites/MapAssets/worlds/";
    std::string ext = ".argmap";

    std::string pathInRoot = baseDir + resolvedMapPath + ext;
    std::string pathInMazmorra = baseDir + "mazmorra/" + resolvedMapPath + ext;
    std::string pathInCaverna = baseDir + "caverna/" + resolvedMapPath + ext;

    if (std::ifstream(pathInMazmorra, std::ios::binary).good())
    {
      resolvedMapPath = pathInMazmorra;
    }
    else if (std::ifstream(pathInCaverna, std::ios::binary).good())
    {
      resolvedMapPath = pathInCaverna;
    }
    else
    {

      resolvedMapPath = pathInRoot;
    }
  }

  {
    std::ifstream check(resolvedMapPath, std::ios::binary);
    if (!check.good())
    {
      world.addPlayer(std::move(*player));

      std::cerr << "[GameLoop] Transición fallida: El mapa no se encontró en "
                   "ninguna carpeta conocida. Ruta intentada: '"
                << resolvedMapPath << "' playerId=" << entry.playerId
                << std::endl;

      if (clientQueue != nullptr)
      {
        monitor.sendTo(entry.playerId,
                       std::make_shared<const ErrorMessage>(
                           "El mapa '" + entry.targetMap +
                           "' no existe en las carpetas del servidor."));
      }
      return;
    }
  }

  uint16_t spawnX = 3;
  uint16_t spawnY = 3;

  monitor.removeQueue(entry.playerId);
  transitionQueue.try_push(std::make_shared<InstanceTransitionEvent>(
      InstanceTransitionEvent{entry.playerId, gameId, std::move(*player),
                              clientQueue, resolvedMapPath, spawnX, spawnY}));
}

void GameLoop::stop()
{
  Thread::stop();
  gameQueue.close();
}
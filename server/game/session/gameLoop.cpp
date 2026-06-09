#include "gameLoop.h"

#include "common/network/messages/server/npc/npcSpawnMessage.h"
#include "common/network/messages/server/npc/npcMoveMessage.h"

GameLoop::GameLoop(Queue<ClientMessage> &q, Monitor &m, GameWorld &w,
                   Queue<std::shared_ptr<LeaveEvent>> &leaveQ, Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQ, uint32_t gameId,
                   const toml::table &config)
    : gameQueue(q), monitor(m), world(w), leaveQueue(leaveQ), transitionQueue(transitionQ), gameId(gameId),
      dispatcher(config),
      statManager(config),
      tickRateMs(config["server"]["tick_rate_ms"].value_or(33)) {}

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

  // Si cambiaron stats de jugadores, mandamos stats actualizadas.
  for (uint32_t id : result.playersChanged)
  {
    statManager.sendPlayerStats(id, world, monitor);
  }
  for (uint32_t deadPlayerId : result.playersDied)
  {
    monitor.broadcast(std::make_shared<const PlayerDiedMessage>(deadPlayerId));

    std::cout << "[GameLoop] broadcast PLAYER_DIED id=" << deadPlayerId << std::endl;
  }

  // Si algún NPC se movió, avisamos al cliente con un mensaje específico.
  // Esto NO crea NPCs. Solo actualiza su posición visual.
  for (uint32_t npcId : result.npcsMoved)
  {
    const Npc &npc = world.getNpc(npcId);

    monitor.broadcast(
        std::make_shared<const NpcMoveMessage>(
            npc.getId(),
            static_cast<uint16_t>(npc.getTileX() * 96),
            static_cast<uint16_t>(npc.getTileY() * 96)));

    std::cout << "[GameLoop] broadcast NPC move id="
              << npc.getId()
              << " pos=("
              << npc.getTileX()
              << ", "
              << npc.getTileY()
              << ")"
              << std::endl;
  }

  for (const auto &npcSpawn : result.spawnedNpcs)
  {
    monitor.broadcast(
        std::make_shared<const NpcSpawnMessage>(
            npcSpawn.npcId,
            npcSpawn.type,
            npcSpawn.name,
            npcSpawn.x,
            npcSpawn.y,
            npcSpawn.hp,
            npcSpawn.maxHp,
            npcSpawn.hostile));

    std::cout << "[GameLoop] broadcast NPC respawn id="
              << npcSpawn.npcId
              << " name="
              << npcSpawn.name
              << std::endl;
  }

  // Transiciones de instancia.
  for (auto &entry : result.instanceTransitions)
  {
    handleInstanceTransition(entry);
  }
}

void GameLoop::handleLeaveGame(uint32_t clientId)
{
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

  uint16_t spawnX = entry.returnTileX;
  uint16_t spawnY = entry.returnTileY;

  if (entry.targetMap.find("mazmorra") != std::string::npos)
  {
    spawnX = 35;
    spawnY = 35;
  }
  else if (entry.targetMap.find("caverna") != std::string::npos)
  {
    spawnX = 25;
    spawnY = 5;
  }
  else
  {
    spawnX = 3;
    spawnY = 3;
  }

  Queue<std::shared_ptr<const Message>> *clientQueue =
      monitor.getQueue(entry.playerId);
  monitor.removeQueue(entry.playerId);

  transitionQueue.try_push(std::make_shared<InstanceTransitionEvent>(
      InstanceTransitionEvent{
          entry.playerId,
          gameId,
          std::move(*player),
          clientQueue,
          entry.targetMap,
          spawnX,
          spawnY}));
}

void GameLoop::stop()
{
  Thread::stop();
  gameQueue.close();
}
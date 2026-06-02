#include "gameLoop.h"


GameLoop::GameLoop(
    Queue<ClientMessage> &gameQueue,
    Monitor &monitor,
    GameWorld &world,
    Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
    Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue,
    uint32_t gameId,
    const toml::table &config)
    : gameQueue(gameQueue),
      monitor(monitor),
      dispatcher(config),
      world(world),
      statManager(),
      leaveQueue(leaveQueue),
      gameId(gameId),
      tickRateMs(50.0f),
      tileSize(32),
      initialSnapshotSent(false),
      transitionQueue(transitionQueue)
{
}

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

            float elapsed =
                std::chrono::duration_cast<Ms>(t2 - t1).count();

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
                std::this_thread::sleep_for(
                    Duration(static_cast<long>(rest)));
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
        std::cerr << "[GameLoop] error: "
                  << e.what()
                  << std::endl;
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
    if (!initialSnapshotSent) {
        sendInitialSnapshot();
        initialSnapshotSent = true;
    }
 
    auto result = world.tick(deltaSeconds);
 
    for (uint32_t id : result.playersChanged)
        statManager.sendPlayerStats(id, world, monitor);
 
    for (auto& entry : result.instanceTransitions)
        handleInstanceTransition(entry);
 
    for (uint32_t npcId : result.npcsMoved) {
        const Npc& npc = world.getNpc(npcId);
 
        // Los pies del NPC ya estan en pixeles — el cliente los usa directamente
        monitor.broadcast(std::make_shared<EntityMoveMessage>(
            static_cast<uint8_t>(npcId),
            static_cast<int16_t>(npc.getPixelX()),
            static_cast<int16_t>(npc.getPixelY())));
    }
 
    for (const auto& death : result.npcDeaths)
        monitor.broadcast(std::make_shared<EntityDespawnMessage>(death.npcId));
 
    for (uint32_t npcId : result.npcSpawned) {
        const Npc& npc = world.getNpc(npcId);
 
        monitor.broadcast(std::make_shared<EntitySpawnMessage>(
            npcId,
            npc.getType(),
            static_cast<uint16_t>(npc.getPixelX()),
            static_cast<uint16_t>(npc.getPixelY())));
    }
}

void GameLoop::sendInitialSnapshot()
{
    const auto& npcs = world.getNpcs();
    std::vector<NpcSnapshot> snapshots;
 
    for (const auto& [id, npc] : npcs) {
        snapshots.push_back({
            id,
            npc.getType(),
            static_cast<uint16_t>(npc.getPixelX()),
            static_cast<uint16_t>(npc.getPixelY())});
    }
 
    monitor.broadcast(std::make_shared<NpcListMessage>(std::move(snapshots)));
}
void GameLoop::handleLeaveGame(uint32_t clientId)
{
    auto player = world.removePlayer(clientId);

    if (!player)
        return;

    Queue<std::shared_ptr<const Message>> *clientQueue =
        monitor.getQueue(clientId);

    monitor.removeQueue(clientId);

    leaveQueue.try_push(
        std::make_shared<LeaveEvent>(
            LeaveEvent{
                clientId,
                gameId,
                std::move(*player),
                clientQueue}));
}

void GameLoop::handleInstanceTransition(
    const GameWorld::InstanceEntry &entry)
{
    auto player = world.removePlayer(entry.playerId);

    if (!player)
        return;

    auto [safeX, safeY] =
        world.findSafeSpawnNear(
            entry.returnTileX,
            entry.returnTileY);

    Queue<std::shared_ptr<const Message>> *clientQueue =
        monitor.getQueue(entry.playerId);

    monitor.removeQueue(entry.playerId);

    transitionQueue.try_push(
        std::make_shared<InstanceTransitionEvent>(
            InstanceTransitionEvent{
                entry.playerId,
                gameId,
                std::move(*player),
                clientQueue,
                entry.targetMap,
                safeX,
                safeY}));
}

void GameLoop::stop()
{
    Thread::stop();
    gameQueue.close();
}
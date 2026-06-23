#include "gameLoop.h"

GameLoop::GameLoop(Queue<ClientMessage> &q, Monitor &m, GameWorld &w,
                   Queue<std::shared_ptr<LeaveEvent>> &leaveQ,
                   Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQ,
                   uint32_t gameId, const toml::table &config,
                   PlayerArchive &archive, const std::string &mapId,
                   ClanManager &clanManager, uint32_t originRoomId)
    : gameQueue(q), monitor(m), world(w), leaveQueue(leaveQ), transitionQueue(transitionQ),
      gameId(gameId), dispatcher(config, clanManager), statManager(config),
      tickRateMs(config["server"]["tick_rate_ms"].value_or(33)),
      archive(archive), mapId(mapId),
      persistEveryNTicks(config["server"]["persist_every_n_ticks"].value_or(300)),
      originRoomId(originRoomId)
{
}

void GameLoop::run()
{
    using Clock = std::chrono::steady_clock;
    using Ms = std::chrono::duration<float, std::milli>;
    using Duration = std::chrono::milliseconds;

    Clock::time_point t1 = Clock::now();

    try
    {
        while (true)
        {
            ClientMessage incoming;
            while (gameQueue.try_pop(incoming))
                processMessage(incoming);

            Clock::time_point t2 = Clock::now();
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

    if (!world.hasPlayer(incoming.clientId))
        return;

    dispatcher.dispatch(incoming, world, monitor);
}

void GameLoop::handleLeaveGame(uint32_t clientId)
{
    std::optional<Player> player = world.removePlayer(clientId);
    if (!player.has_value())
        return;

    Queue<std::shared_ptr<const Message>> *clientQueue = monitor.getQueue(clientId);
    monitor.removeQueue(clientId);

    leaveQueue.try_push(std::make_shared<LeaveEvent>(
        LeaveEvent{clientId, gameId, std::move(*player), clientQueue}));
}

void GameLoop::handleInstanceTransition(const InstanceEntry &entry)
{
    std::optional<Player> player = world.removePlayer(entry.playerId);
    if (!player.has_value())
        return;

    monitor.broadcastExcept(entry.playerId,
                            std::make_shared<const EntityDespawnMessage>(entry.playerId));

    Queue<std::shared_ptr<const Message>> *clientQueue = monitor.getQueue(entry.playerId);

    if (entry.targetMap.empty())
    {
        monitor.removeQueue(entry.playerId);
        transitionQueue.try_push(std::make_shared<InstanceTransitionEvent>(
            InstanceTransitionEvent{
                entry.playerId, gameId, std::move(*player),
                clientQueue, entry.targetMap, 3, 3}));
        return;
    }

    std::string resolvedMapPath = resolveMapPath(entry.targetMap);

    {
        std::ifstream check(resolvedMapPath, std::ios::binary);
        if (!check.good())
        {
            world.addPlayer(std::move(*player));
            if (clientQueue != nullptr)
            {
                monitor.sendTo(entry.playerId,
                               std::make_shared<const ErrorMessage>(
                                   "Map '" + entry.targetMap + "' not found on server."));
            }
            return;
        }
    }

    const uint16_t spawnX = 3;
    const uint16_t spawnY = 3;

    monitor.removeQueue(entry.playerId);
    transitionQueue.try_push(std::make_shared<InstanceTransitionEvent>(
        InstanceTransitionEvent{
            entry.playerId, gameId, std::move(*player),
            clientQueue, resolvedMapPath, spawnX, spawnY}));
}

void GameLoop::stop()
{
    Thread::stop();
    gameQueue.close();
}

std::string GameLoop::resolveMapPath(const std::string &targetMap) const
{
    if (targetMap.find("assets/") != std::string::npos)
        return targetMap;

    const std::string baseDir = "assets/sprites/MapAssets/worlds/";
    const std::string ext = ".argmap";

    const std::string pathInMazmorra = baseDir + "mazmorra/" + targetMap + ext;
    const std::string pathInCaverna  = baseDir + "caverna/"  + targetMap + ext;
    const std::string pathInRoot     = baseDir + targetMap + ext;

    if (std::ifstream(pathInMazmorra, std::ios::binary).good())
        return pathInMazmorra;
    if (std::ifstream(pathInCaverna, std::ios::binary).good())
        return pathInCaverna;
    return pathInRoot;
}

void GameLoop::worldUpdate(float deltaSeconds)
{
    WorldTickResult result = world.tick(deltaSeconds);

    for (const WorldTickResult::ResurrectStartedInfo &info : result.resurrectionStarted)
    {
        monitor.sendTo(info.playerId,
                       std::make_shared<ResurrectionStartedMessage>(info.delayMs));
    }

    for (const WorldTickResult::PlayerHit &hit : result.playerHits)
    {
        statManager.sendPlayerStats(hit.playerId, world, monitor);
        monitor.sendTo(hit.playerId,
                       std::make_shared<const ChatNotificationMessage>(
                           "Un enemigo te causó " + std::to_string(hit.damage) +
                               " puntos de daño.",
                           ChatMsgType::DAMAGE_TAKEN));
    }

    for (const WorldTickResult::ClanAllyHit &allyHit : result.clanAllyHits)
    {
        std::vector<uint32_t> allies = world.getOnlineClanMemberIds(allyHit.clanName);
        const std::string msg = "¡Nuestro aliado " + allyHit.targetName + " está siendo atacado!";
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
        if (world.hasPlayer(id))
        {
            Player &player = world.getPlayer(id);
            monitor.broadcast(std::make_shared<const PlayerHealthMessage>(
                id,
                static_cast<uint16_t>(player.getHp()),
                static_cast<uint16_t>(player.getMaxHp())));
        }
    }

    for (const std::pair<uint32_t, DeathResult> &entry : result.playerDeathsByNpc)
    {
        const uint32_t playerId = entry.first;
        const DeathResult &deathResult = entry.second;

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
                item, deathResult.tileX, deathResult.tileY));
        }

        Player &victim = world.getPlayer(playerId);
        monitor.sendTo(playerId,
                       std::make_shared<const InventoryUpdateMessage>(
                           victim.getInventory().getItems(),
                           victim.getInventory().getInventorySlots(),
                           victim.getInventory().getEquippedArray()));
        monitor.broadcast(std::make_shared<const PlayerHealthMessage>(
            playerId,
            static_cast<uint16_t>(victim.getHp()),
            static_cast<uint16_t>(victim.getMaxHp())));
    }

    for (uint32_t deadPlayerId : result.playersDied)
    {
        monitor.broadcast(std::make_shared<const PlayerDiedMessage>(deadPlayerId));
    }

    for (const WorldTickResult::PlayerResurrection &res : result.playersResurrected)
    {
        monitor.broadcast(std::make_shared<const PlayerResurrectedMessage>(
            res.playerId, res.tileX, res.tileY));
        monitor.broadcast(std::make_shared<const EntityMoveMessage>(
            static_cast<uint32_t>(res.playerId),
            world.getPixelX(res.playerId),
            world.getPixelY(res.playerId),
            Direction::DOWN,
            false));
    }

    for (const WorldTickResult::NpcAttackAnim &atk : result.npcAttacksForAnim)
    {
        monitor.broadcast(
            std::make_shared<const NpcAttackMessage>(atk.npcId, atk.direction));
    }

    for (uint32_t npcId : result.npcsMoved)
    {
        const Npc &npc = world.getNpc(npcId);
        monitor.broadcast(std::make_shared<const NpcMoveMessage>(
            npc.getId(),
            static_cast<uint16_t>(npc.getTileX() * 96),
            static_cast<uint16_t>(npc.getTileY() * 96)));
    }

    for (const NpcSpawnEvent &npcSpawn : result.spawnedNpcs)
    {
        monitor.broadcast(std::make_shared<const NpcSpawnMessage>(
            npcSpawn.npcId,
            npcSpawn.type,
            npcSpawn.name,
            npcSpawn.x,
            npcSpawn.y,
            npcSpawn.hp,
            npcSpawn.maxHp,
            npcSpawn.level,
            npcSpawn.hostile));
    }

    for (const InstanceEntry &entry : result.instanceTransitions)
    {
        handleInstanceTransition(entry);
    }

    ++persistTickCounter;
    if (persistTickCounter >= persistEveryNTicks)
    {
        persistTickCounter = 0;
        for (const std::pair<const uint32_t, Player> &entry : world.getPlayers())
        {
            archive.enqueue(
                archive.toSnapshot(entry.second, mapId, gameId, originRoomId),
                gameId);
        }
    }
}
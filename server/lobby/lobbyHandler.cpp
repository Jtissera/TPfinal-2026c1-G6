#include "lobbyHandler.h"

LobbyHandler::LobbyHandler(
    Queue<ClientMessage> &lobbyQueue,
    Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
    Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue,
    Monitor &lobbyMonitor,
    GameManager &gameManager,
    ReceiverRegistry &receiverRegistry,
    PlayerRepository &playerRepo,
    PlayerFactory &playerFactory,
    PlayerArchive &archive,
    CharacterArchive &characterArchive,
    const toml::table &config)
    : lobbyQueue(lobbyQueue), leaveQueue(leaveQueue), transitionQueue(transitionQueue),
      lobbyMonitor(lobbyMonitor), gameManager(gameManager), receiverRegistry(receiverRegistry),
      playerRepo(playerRepo), playerFactory(playerFactory), archive(archive), characterArchive(characterArchive), config(config)
{
  initHandlers();
}

void LobbyHandler::initHandlers()
{
  handlers[static_cast<uint8_t>(ClientOpCode::MSG_CONNECT)] = &LobbyHandler::handleConnect;
  handlers[static_cast<uint8_t>(ClientOpCode::MSG_LOGIN)] = &LobbyHandler::handleLogin;
  handlers[static_cast<uint8_t>(ClientOpCode::MSG_CREATE_CHAR)] = &LobbyHandler::handleCreateChar;
  handlers[static_cast<uint8_t>(ClientOpCode::MSG_LIST_GAMES)] = &LobbyHandler::handleListGames;
  handlers[static_cast<uint8_t>(ClientOpCode::MSG_CREATE_GAME)] = &LobbyHandler::handleCreateGame;
  handlers[static_cast<uint8_t>(ClientOpCode::MSG_JOIN_GAME)] = &LobbyHandler::handleJoinGame;
}

void LobbyHandler::run()
{
  while (true)
  {
    try
    {
      ClientMessage incoming;
      while (lobbyQueue.try_pop(incoming))
      {
        uint8_t opcode = incoming.message->opCode();
        std::unordered_map<uint8_t, MemberHandler>::iterator it =
            handlers.find(opcode);
        if (it != handlers.end())
        {
          (this->*it->second)(incoming.clientId, *incoming.message);
        }
      }

      std::shared_ptr<LeaveEvent> leaveEvent;
      while (leaveQueue.try_pop(leaveEvent))
      {
        handleLeaveGame(*leaveEvent);
      }

      std::shared_ptr<InstanceTransitionEvent> transition;
      while (transitionQueue.try_pop(transition))
      {
        try
        {
          handleInstanceTransition(*transition);
        }
        catch (const std::exception &e)
        {
          std::cerr << "[LobbyHandler] Error in client transition "
                    << transition->clientId << ": " << e.what()
                    << std::endl;
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    catch (const ClosedQueue &)
    {
      break;
    }
    catch (const std::exception &e)
    {
      std::cerr << "[LobbyHandler] Critical error in loop: " << e.what()
                << std::endl;
    }
  }
}

void LobbyHandler::stop()
{
  Thread::stop();
  lobbyQueue.close();
  leaveQueue.close();
}

void LobbyHandler::handleConnect(uint32_t clientId, const Message &message)
{
  const ConnectMessage &connectMsg = static_cast<const ConnectMessage &>(message);
  std::cout << "[LobbyHandler] client=" << clientId
            << " username=" << connectMsg.getUsername() << " connected"
            << std::endl;

  lobbyMonitor.sendTo(clientId, std::make_shared<const ConnectOkMessage>());
}

void LobbyHandler::handleLogin(uint32_t clientId, const Message &message)
{
  const LoginMessage &msg = static_cast<const LoginMessage &>(message);
  const std::string &characterName = msg.getCharacterName();

  if (!characterArchive.exists(characterName))
  {
    lobbyMonitor.sendTo(clientId,
                        std::make_shared<const ErrorMessage>(
                            "Character '" + characterName + "' not found."));
    return;
  }

  if (!gameManager.tryMarkOnline(clientId, characterName))
  {
    lobbyMonitor.sendTo(clientId,
                        std::make_shared<const ErrorMessage>(
                            "That character is already connected."));
    return;
  }

  pendingCharacterNames[clientId] = characterName;
  lobbyMonitor.sendTo(clientId, std::make_shared<LoginOkMessage>());
  std::cout << "[LobbyHandler] Login OK: '" << characterName
            << "' clientId=" << clientId << std::endl;
}

void LobbyHandler::handleCreateChar(uint32_t clientId, const Message &message)
{
  const CreateCharMessage &msg = static_cast<const CreateCharMessage &>(message);
  try
  {
    if (!characterArchive.save(msg.getName(), msg.getRace(), msg.getCharacterClass()))
    {
      lobbyMonitor.sendTo(clientId,
                          std::make_shared<const ErrorMessage>(
                              "Name '" + msg.getName() + "' is already taken."));
      return;
    }

    gameManager.tryMarkOnline(clientId, msg.getName());

    Player player = playerFactory.create(
        clientId, msg.getName(), msg.getRace(), msg.getCharacterClass(), 2, 2);
    playerRepo.save(clientId, std::move(player));
    lobbyMonitor.sendTo(clientId, std::make_shared<const CreateOkMessage>());
  }
  catch (const std::exception &e)
  {
    lobbyMonitor.sendTo(clientId,
                        std::make_shared<const ErrorMessage>(e.what()));
  }
}

void LobbyHandler::handleListGames(uint32_t clientId, const Message &)
{
  std::vector<GameInfo> games = gameManager.listGames();
  lobbyMonitor.sendTo(clientId,
                      std::make_shared<const GameListMessage>(std::move(games)));
}

void LobbyHandler::handleCreateGame(uint32_t clientId, const Message &message)
{
  const CreateGameMessage &createMsg =
      static_cast<const CreateGameMessage &>(message);
  try
  {
    uint32_t gameId = gameManager.createGame(
        createMsg.getGameName(),
        createMsg.getMaxPlayers(),
        createMsg.getMapPath());
    lobbyMonitor.sendTo(clientId,
                        std::make_shared<const GameCreatedMessage>(
                            gameId,
                            createMsg.getGameName(),
                            createMsg.getMaxPlayers()));
  }
  catch (const std::exception &e)
  {
    std::cerr << "[LobbyHandler] createGame failed: " << e.what() << std::endl;
    lobbyMonitor.sendTo(clientId,
                        std::make_shared<const ErrorMessage>(e.what()));
  }
}

void LobbyHandler::handleJoinGame(uint32_t clientId, const Message &message)
{
  const JoinGameMessage &joinMsg = static_cast<const JoinGameMessage &>(message);
  const uint32_t requestedGameId = joinMsg.getGameId();

  Queue<std::shared_ptr<const Message>> *clientQueue =
      lobbyMonitor.getQueue(clientId);
  if (!clientQueue)
  {
    lobbyMonitor.sendTo(clientId,
                        std::make_shared<const ErrorMessage>(
                            "Internal error: client queue not found"));
    return;
  }

  uint32_t targetGameId = requestedGameId;
  Player *player = playerRepo.get(clientId);

  if (!player)
  {
    std::unordered_map<uint32_t, std::string>::iterator it =
        pendingCharacterNames.find(clientId);
    if (it == pendingCharacterNames.end())
    {
      lobbyMonitor.sendTo(clientId,
                          std::make_shared<const ErrorMessage>(
                              "Must login or create a character before joining"));
      return;
    }

    const std::string characterName = it->second;
    pendingCharacterNames.erase(it);

    player = resolveReturningPlayer(
        clientId, characterName, requestedGameId, targetGameId);

    if (!player)
    {
      lobbyMonitor.sendTo(clientId,
                          std::make_shared<const ErrorMessage>(
                              "Internal error: failed to load character."));
      return;
    }
  }

  finalizeJoin(clientId, targetGameId, requestedGameId, *player, *clientQueue);
}

Player *LobbyHandler::resolveReturningPlayer(uint32_t clientId,
                                             const std::string &characterName,
                                             uint32_t requestedGameId,
                                             uint32_t &targetGameId)
{
  std::optional<PlayerSnapshot> snap =
      archive.loadSnapshot(characterName, requestedGameId);

  if (snap.has_value())
  {
    targetGameId = resolveTargetInstance(
        characterName, requestedGameId, snap.value());

    std::optional<Player> loaded =
        archive.load(characterName, requestedGameId);
    if (loaded.has_value())
    {
      loaded.value().setClientId(clientId);
      playerRepo.save(clientId, std::move(loaded.value()));
    }
  }
  else
  {
    std::optional<CharacterRecord> rec =
        characterArchive.load(characterName);
    if (!rec.has_value())
    {
      return nullptr;
    }

    std::string race(rec->race, strnlen(rec->race, sizeof(rec->race)));
    std::string cls(rec->cls, strnlen(rec->cls, sizeof(rec->cls)));

    Player fresh = playerFactory.create(
        clientId, characterName, race, cls, 3, 3);
    std::cout << "[LobbyHandler] '" << characterName
              << "' entering game " << requestedGameId
              << " for the first time" << std::endl;
    playerRepo.save(clientId, std::move(fresh));
  }

  return playerRepo.get(clientId);
}

uint32_t LobbyHandler::resolveTargetInstance(const std::string &characterName,
                                             uint32_t requestedGameId,
                                             const PlayerSnapshot &snap)
{
  if (snap.originGameId == 0)
  {
    return requestedGameId;
  }

  std::string instanceMap(snap.mapId, strnlen(snap.mapId, sizeof(snap.mapId)));
  try
  {
    uint32_t instanceId =
        gameManager.getOrCreateInstance(instanceMap, requestedGameId);
    std::cout << "[LobbyHandler] '" << characterName
              << "' reconnecting to instance gameId=" << instanceId
              << " map='" << instanceMap << "'" << std::endl;
    return instanceId;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[LobbyHandler] Could not recreate instance for '"
              << characterName << "': " << e.what()
              << " — falling back to public room." << std::endl;
    return requestedGameId;
  }
}

void LobbyHandler::finalizeJoin(uint32_t clientId,
                                uint32_t targetGameId,
                                uint32_t requestedGameId,
                                Player &player,
                                Queue<std::shared_ptr<const Message>> &clientQueue)
{

  if (!gameManager.joinGame(targetGameId, clientId, clientQueue))
  {
    lobbyMonitor.sendTo(clientId,
                        std::make_shared<const ErrorMessage>("Game not found or full"));
    return;
  }

  PlayerDto playerDto = buildPlayerDto(player);
  std::string mapId = gameManager.getRoomMapPath(targetGameId);
  uint32_t originId = gameManager.getOriginRoomId(targetGameId);

  archive.enqueue(
      archive.toSnapshot(player, mapId, targetGameId, originId),
      targetGameId);

  gameManager.addPlayerToGame(targetGameId, std::move(player));
  playerRepo.remove(clientId);

  std::string gameName = findGameName(requestedGameId);

  clientQueue.try_push(std::make_shared<const JoinOkMessage>(
      targetGameId, gameName, std::move(playerDto)));

  if (targetGameId != requestedGameId)
  {
    clientQueue.try_push(
        std::make_shared<const MapChangedMessage>(mapId));
  }

  gameManager.syncPlayerJoin(targetGameId, clientId);

  Receiver *receiver = receiverRegistry.get(clientId);
  if (receiver)
  {
    receiver->setQueue(gameManager.getGameQueue(targetGameId));
  }

  lobbyMonitor.removeQueue(clientId);
}

void LobbyHandler::handleLeaveGame(LeaveEvent &event)
{
  gameManager.removeClient(event.clientId);

  Receiver *receiver = receiverRegistry.get(event.clientId);
  if (receiver)
    receiver->setQueue(lobbyQueue);

  lobbyMonitor.addQueue(event.clientId, *event.clientQueue);
  playerRepo.save(event.clientId, std::move(event.player));
  lobbyMonitor.sendTo(event.clientId, std::make_shared<const LeaveOkMessage>());
}

void LobbyHandler::handleInstanceTransition(InstanceTransitionEvent &event)
{
  if (event.targetMap.empty())
  {
    transitionToOrigin(event);
  }
  else
  {
    transitionToInstance(event);
  }
}

void LobbyHandler::transitionToOrigin(InstanceTransitionEvent &event)
{
  uint32_t originId = gameManager.getOriginRoomId(event.fromRoomId);
  if (originId == 0)
  {
    return;
  }

  gameManager.broadcastDespawnInRoom(event.fromRoomId, event.clientId);
  gameManager.unregisterClientForTransition(event.clientId);
  event.player.setTilePos(event.spawnTileX, event.spawnTileY);

  std::string originMapPath = gameManager.getRoomMapPath(originId);
  completeTransition(originId, event, originMapPath, 0);
}

void LobbyHandler::transitionToInstance(InstanceTransitionEvent &event)
{
  std::string fullMapPath = buildFullMapPath(event.targetMap);

  uint32_t instanceId =
      gameManager.getOrCreateInstance(fullMapPath, event.fromRoomId);

  gameManager.broadcastDespawnInRoom(event.fromRoomId, event.clientId);
  gameManager.unregisterClientForTransition(event.clientId);
  event.player.setTilePos(event.spawnTileX, event.spawnTileY);

  completeTransition(instanceId, event, fullMapPath, event.fromRoomId);
}

void LobbyHandler::completeTransition(uint32_t targetRoomId,
                                      InstanceTransitionEvent &event,
                                      const std::string &mapPath,
                                      uint32_t originId)
{
  archive.enqueue(
      archive.toSnapshot(event.player, mapPath, targetRoomId, originId),
      targetRoomId);

  gameManager.joinAndAddPlayer(
      targetRoomId, event.clientId, *event.clientQueue, std::move(event.player));

  event.clientQueue->try_push(
      std::make_shared<const MapChangedMessage>(mapPath));

  gameManager.syncPlayerJoin(targetRoomId, event.clientId);

  Receiver *receiver = receiverRegistry.get(event.clientId);
  if (receiver)
  {
    receiver->setQueue(gameManager.getGameQueue(targetRoomId));
  }
}

std::string LobbyHandler::findGameName(uint32_t gameId) const
{
  std::vector<GameInfo> games = gameManager.listGames();
  for (std::vector<GameInfo>::const_iterator it = games.begin();
       it != games.end(); ++it)
  {
    if (it->gameId == gameId)
    {
      return it->gameName;
    }
  }
  return "";
}

std::string LobbyHandler::buildFullMapPath(const std::string &targetMap) const
{
  if (targetMap.find("assets/") != std::string::npos)
  {
    return targetMap;
  }
  return "assets/sprites/MapAssets/worlds" + targetMap + ".argmap";
}

PlayerDto LobbyHandler::buildPlayerDto(const Player &player) const
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
  dto.expMax = defaultExpMax;
  dto.esFantasma = player.isGhost();
  dto.fuerza = player.getStrength();
  dto.agilidad = player.getAgility();
  dto.inteligencia = defaultIntelligence;
  dto.constitucion = defaultConstitution;

  std::cout << "[LobbyHandler] PlayerDto built: race='" << dto.raza
            << "' class='" << dto.clase << "'" << std::endl;

  return dto;
}
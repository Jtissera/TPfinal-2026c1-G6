#include "lobbyHandler.h"

#include "../common/network/messages/client/auth/loginMessage.h"
#include "../common/network/messages/client/lobby/createGameMessage.h"
#include "../common/network/messages/client/lobby/joinGameMessage.h"
#include "../common/network/messages/client/lobby/listGamesMessage.h"
#include "../common/network/messages/server/auth/loginOkMessage.h"

LobbyHandler::LobbyHandler(
    Queue<ClientMessage> &lobbyQueue,
    Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
    Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue,
    Monitor &lobbyMonitor, GameManager &gameManager,
    ReceiverRegistry &receiverRegistry, PlayerRepository &playerRepo,
    PlayerFactory &playerFactory, PlayerArchive &archive,
    CharacterArchive &characterArchive,
    const toml::table &config)
    : lobbyQueue(lobbyQueue), leaveQueue(leaveQueue),
      transitionQueue(transitionQueue), lobbyMonitor(lobbyMonitor),
      gameManager(gameManager), receiverRegistry(receiverRegistry),
      playerRepo(playerRepo), playerFactory(playerFactory), archive(archive),
      characterArchive(characterArchive), config(config)
{
  initHandlers();
}

void LobbyHandler::initHandlers()
{
  handlers[static_cast<uint8_t>(ClientOpCode::MSG_CONNECT)] =
      [this](uint32_t id, const Message &msg)
  { handleConnect(id, msg); };

  handlers[static_cast<uint8_t>(ClientOpCode::MSG_LOGIN)] =
      [this](uint32_t id, const Message &msg)
  { handleLogin(id, msg); };

  handlers[static_cast<uint8_t>(ClientOpCode::MSG_CREATE_CHAR)] =
      [this](uint32_t id, const Message &msg)
  { handleCreateChar(id, msg); };

  handlers[static_cast<uint8_t>(ClientOpCode::MSG_LIST_GAMES)] =
      [this](uint32_t id, const Message &msg)
  { handleListGames(id, msg); };

  handlers[static_cast<uint8_t>(ClientOpCode::MSG_CREATE_GAME)] =
      [this](uint32_t id, const Message &msg)
  { handleCreateGame(id, msg); };

  handlers[static_cast<uint8_t>(ClientOpCode::MSG_JOIN_GAME)] =
      [this](uint32_t id, const Message &msg)
  { handleJoinGame(id, msg); };
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
        auto it = handlers.find(opcode);
        if (it != handlers.end())
          it->second(incoming.clientId, *incoming.message);
      }

      std::shared_ptr<LeaveEvent> leaveEvent;
      while (leaveQueue.try_pop(leaveEvent))
        handleLeaveGame(*leaveEvent);

      std::shared_ptr<InstanceTransitionEvent> transition;
      while (transitionQueue.try_pop(transition))
      {
        try
        {
          handleInstanceTransition(*transition);
        }
        catch (const std::exception &e)
        {
          std::cerr << "[LobbyHandler] Error en transicion de cliente "
                    << transition->clientId << ": " << e.what() << std::endl;
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
      std::cerr << "[LobbyHandler] Error crítico en el loop: " << e.what()
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

// ─── handlers ────────────────────────────────────────────────────────────────

void LobbyHandler::handleConnect(uint32_t clientId, const Message &message)
{
  const auto &connectMsg = static_cast<const ConnectMessage &>(message);
  std::cout << "[LobbyHandler] client=" << clientId
            << " username=" << connectMsg.getUsername() << " connected"
            << std::endl;

  lobbyMonitor.sendTo(clientId, std::make_shared<const ConnectOkMessage>());
}

void LobbyHandler::handleCreateChar(uint32_t clientId, const Message &message)
{
  const auto &msg = static_cast<const CreateCharMessage &>(message);
  try
  {
    // Unicidad: si el nombre ya existe, rechazamos
    if (!characterArchive.save(msg.getName(), msg.getRaza(), msg.getClase()))
    {
      lobbyMonitor.sendTo(
          clientId, std::make_shared<const ErrorMessage>(
                        "El nombre '" + msg.getName() + "' ya está en uso."));
      return;
    }

    gameManager.tryMarkOnline(clientId, msg.getName());

    Player player = playerFactory.create(clientId, msg.getName(), msg.getRaza(),
                                         msg.getClase(), 2, 2);
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
  auto games = gameManager.listGames();
  lobbyMonitor.sendTo(
      clientId, std::make_shared<const GameListMessage>(std::move(games)));
}

void LobbyHandler::handleLogin(uint32_t clientId, const Message &message)
{
  const auto &msg = static_cast<const LoginMessage &>(message);
  const std::string &characterName = msg.getCharacterName();

  if (!characterArchive.exists(characterName))
  {
    lobbyMonitor.sendTo(
        clientId, std::make_shared<const ErrorMessage>(
                      "Personaje '" + characterName + "' no encontrado."));
    return;
  }

  if (!gameManager.tryMarkOnline(clientId, characterName))
  {
    lobbyMonitor.sendTo(
        clientId, std::make_shared<const ErrorMessage>(
                      "Ese personaje ya está conectado."));
    return;
  }

  pendingCharacterNames[clientId] = characterName;
  lobbyMonitor.sendTo(clientId, std::make_shared<LoginOkMessage>());
  std::cout << "[LobbyHandler] Login OK: '" << characterName
            << "' clientId=" << clientId << std::endl;
}

void LobbyHandler::handleCreateGame(uint32_t clientId, const Message &message)
{
  const auto &createMsg = static_cast<const CreateGameMessage &>(message);
  try
  {
    auto gameId = gameManager.createGame(createMsg.getGameName(),
                                         createMsg.getMaxPlayers(),
                                         createMsg.getMapPath());
    lobbyMonitor.sendTo(clientId, std::make_shared<const GameCreatedMessage>(
                                      gameId, createMsg.getGameName(),
                                      createMsg.getMaxPlayers()));
  }
  catch (const std::exception &e)
  {
    std::cerr << "[SERVER] createGame falló: " << e.what() << std::endl;
    lobbyMonitor.sendTo(clientId,
                        std::make_shared<const ErrorMessage>(e.what()));
  }
}

void LobbyHandler::handleJoinGame(uint32_t clientId, const Message &message)
{
  const auto &joinMsg = static_cast<const JoinGameMessage &>(message);
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

  // ── Resolución del Player ──────────────────────────────────────────────
  // Caso A: personaje recién creado (está en playerRepo, nunca fue persistido).
  // Caso B: personaje existente que hizo login (nombre en
  // pendingCharacterNames, hay que cargarlo del archive con este gameId).

  Player *player = playerRepo.get(clientId);

  // targetGameId puede diferir de requestedGameId si el jugador estaba en
  // una instancia al desconectarse — en ese caso la recreamos y lo mandamos
  // directamente ahí.
  uint32_t targetGameId = requestedGameId;

  if (!player)
  {
    // Caso B: viene de un login
    auto it = pendingCharacterNames.find(clientId);
    if (it == pendingCharacterNames.end())
    {
      lobbyMonitor.sendTo(
          clientId, std::make_shared<const ErrorMessage>(
                        "Must login or create a character before joining"));
      return;
    }

    const std::string &characterName = it->second;

    if (auto snap = archive.loadSnapshot(characterName, requestedGameId))
    {
      // El jugador ya estuvo en esta sala pública (o en una instancia de ella).
      if (snap->originGameId != 0)
      {
        // Estaba en una instancia cuando se desconectó: recrearla.
        std::string instanceMap(snap->mapId,
                                strnlen(snap->mapId, sizeof(snap->mapId)));
        try
        {
          targetGameId =
              gameManager.getOrCreateInstance(instanceMap, requestedGameId);
          std::cout << "[LobbyHandler] '" << characterName
                    << "' reconectando a instancia gameId=" << targetGameId
                    << " map='" << instanceMap << "'" << std::endl;
        }
        catch (const std::exception &e)
        {
          // El mapa de la instancia ya no existe en disco (caso raro).
          // Fallback: unirse a la sala pública normalmente.
          std::cerr << "[LobbyHandler] No se pudo recrear instancia para '"
                    << characterName << "': " << e.what()
                    << " — fallback a sala pública." << std::endl;
          targetGameId = requestedGameId;
        }
      }

      auto loaded = archive.load(characterName, requestedGameId);
      if (loaded)
      {
        loaded->setClientId(clientId);
        playerRepo.save(clientId, std::move(*loaded));
      }
    }
    else
    {
      // Primera vez en esta sala: construir desde cero con raza/clase del registro
      auto rec = characterArchive.load(characterName);
      if (!rec)
      {
        lobbyMonitor.sendTo(clientId,
                            std::make_shared<const ErrorMessage>(
                                "Error interno al cargar el personaje."));
        pendingCharacterNames.erase(it);
        return;
      }

      Player fresh = playerFactory.create(
          clientId, characterName,
          std::string(rec->race, strnlen(rec->race, sizeof(rec->race))),
          std::string(rec->cls, strnlen(rec->cls, sizeof(rec->cls))), 3, 3);

      std::cout << "[LobbyHandler] '" << characterName
                << "' entra por primera vez a gameId=" << requestedGameId
                << std::endl;
      playerRepo.save(clientId, std::move(fresh));
    }

    pendingCharacterNames.erase(it);
    player = playerRepo.get(clientId);
  }

  // ── Unirse a la GameRoom (pública o instancia recreada) ───────────────────

  if (!gameManager.joinGame(targetGameId, clientId, *clientQueue))
  {
    lobbyMonitor.sendTo(clientId, std::make_shared<const ErrorMessage>(
                                      "Game not found or full"));
    return;
  }

  PlayerDto playerDto = buildPlayerDto(*player);

  std::string mapId = gameManager.getRoomMapPath(targetGameId);
  uint32_t originId = gameManager.getOriginRoomId(targetGameId);
  archive.enqueue(
      archive.toSnapshot(*player, mapId, targetGameId, originId),
      targetGameId);

  gameManager.addPlayerToGame(targetGameId, std::move(*player));
  playerRepo.remove(clientId);

  // El nombre de la partida siempre viene de la sala pública.
  std::string gameName;
  for (const auto &info : gameManager.listGames())
  {
    if (info.gameId == requestedGameId)
    {
      gameName = info.gameName;
      break;
    }
  }

  clientQueue->try_push(std::make_shared<const JoinOkMessage>(
      targetGameId, gameName, std::move(playerDto)));

  if (targetGameId != requestedGameId)
  {
    clientQueue->try_push(std::make_shared<const MapChangedMessage>(mapId));
    std::cout << "[LobbyHandler] MapChanged enviado a clientId=" << clientId
              << " (reconexión a instancia) mapId='" << mapId << "'"
                  << std::endl;
  }

  auto *receiver = receiverRegistry.get(clientId);
  if (receiver)
    receiver->setQueue(gameManager.getGameQueue(targetGameId));

  lobbyMonitor.removeQueue(clientId);
  gameManager.syncPlayerJoin(targetGameId, clientId);
}

void LobbyHandler::handleLeaveGame(LeaveEvent &event)
{
  gameManager.removeClient(event.clientId);

  auto *receiver = receiverRegistry.get(event.clientId);
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
    uint32_t originId = gameManager.getOriginRoomId(event.fromRoomId);
    if (originId == 0)
      return;

    gameManager.broadcastDespawnInRoom(event.fromRoomId, event.clientId);
    gameManager.unregisterClientForTransition(event.clientId);

    event.player.setTilePos(event.spawnTileX, event.spawnTileY);

    std::string originMapPath = gameManager.getRoomMapPath(originId);

    archive.enqueue(
        archive.toSnapshot(event.player, originMapPath, originId, 0),
        originId);

    gameManager.joinAndAddPlayer(originId, event.clientId,
                                 *event.clientQueue, std::move(event.player));

    auto *receiver = receiverRegistry.get(event.clientId);
    if (receiver)
      receiver->setQueue(gameManager.getGameQueue(originId));

    event.clientQueue->try_push(
        std::make_shared<const MapChangedMessage>(originMapPath));

    gameManager.syncPlayerJoin(originId, event.clientId);
  }
  else
  {
    std::string fullMapPath = event.targetMap;
    if (fullMapPath.find("assets/") == std::string::npos)
      fullMapPath = "assets/sprites/MapAssets/worlds" + fullMapPath + ".argmap";

    uint32_t instanceId =
        gameManager.getOrCreateInstance(fullMapPath, event.fromRoomId);

    gameManager.broadcastDespawnInRoom(event.fromRoomId, event.clientId);
    gameManager.unregisterClientForTransition(event.clientId);

    event.player.setTilePos(event.spawnTileX, event.spawnTileY);

    archive.enqueue(
        archive.toSnapshot(event.player, fullMapPath, instanceId,
                           event.fromRoomId),
        instanceId);

    gameManager.joinAndAddPlayer(instanceId, event.clientId,
                                 *event.clientQueue, std::move(event.player));

    auto *receiver = receiverRegistry.get(event.clientId);
    if (receiver)
      receiver->setQueue(gameManager.getGameQueue(instanceId));

    event.clientQueue->try_push(
        std::make_shared<const MapChangedMessage>(fullMapPath));

    gameManager.syncPlayerJoin(instanceId, event.clientId);
  }
}

// ─── helpers ─────────────────────────────────────────────────────────────────

PlayerDto LobbyHandler::buildPlayerDto(const Player &player) const
{
  PlayerDto dto{};

  dto.playerID = player.getClientId();
  dto.nombre = player.getName();
  dto.raza = player.getRace().name;
  dto.clase = player.getCls().name;
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
  dto.expMax = 1000;
  dto.esFantasma = player.isGhost();
  dto.fuerza = player.getStrength();
  dto.agilidad = player.getAgility();
  dto.inteligencia = 10;
  dto.constitucion = 10;

  std::cout << "[SERVER DTO] raza='" << dto.raza << "' clase='" << dto.clase
            << "'" << std::endl;

  return dto;
}
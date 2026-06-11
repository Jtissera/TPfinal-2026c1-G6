#include "lobbyHandler.h"

#include "../common/network/messages/client/lobby/createGameMessage.h"
#include "../common/network/messages/client/lobby/joinGameMessage.h"
#include "../common/network/messages/client/lobby/listGamesMessage.h"

LobbyHandler::LobbyHandler(Queue<ClientMessage> &lobbyQueue,
                           Queue<std::shared_ptr<LeaveEvent>> &leaveQueue,
                           Queue<std::shared_ptr<InstanceTransitionEvent>> &transitionQueue,
                           Monitor &lobbyMonitor, GameManager &gameManager,
                           ReceiverRegistry &receiverRegistry,
                           PlayerRepository &playerRepo,
                           PlayerFactory &playerFactory)
    : lobbyQueue(lobbyQueue), leaveQueue(leaveQueue), transitionQueue(transitionQueue),
      lobbyMonitor(lobbyMonitor), gameManager(gameManager),
      receiverRegistry(receiverRegistry), playerRepo(playerRepo),
      playerFactory(playerFactory)
{
  initHandlers();
}

void LobbyHandler::initHandlers()
{
  handlers[static_cast<uint8_t>(ClientOpCode::MSG_CONNECT)] =
      [this](uint32_t id, const Message &msg)
  { handleConnect(id, msg); };

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
      std::cerr << "[LobbyHandler] Error crítico en el loop: " << e.what() << std::endl;
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

void LobbyHandler::handleCreateGame(uint32_t clientId, const Message &message)
{
  const auto &createMsg = static_cast<const CreateGameMessage &>(message);
  uint32_t gameId = gameManager.createGame(createMsg.getGameName(),
                                           createMsg.getMaxPlayers());
  lobbyMonitor.sendTo(clientId, std::make_shared<const GameCreatedMessage>(
                                    gameId, createMsg.getGameName(),
                                    createMsg.getMaxPlayers()));
}

void LobbyHandler::handleJoinGame(uint32_t clientId, const Message &message)
{
  const auto &joinMsg = static_cast<const JoinGameMessage &>(message);
  const uint32_t gameId = joinMsg.getGameId();

  Queue<std::shared_ptr<const Message>> *clientQueue =
      lobbyMonitor.getQueue(clientId);

  if (!clientQueue)
  {
    lobbyMonitor.sendTo(
        clientId,
        std::make_shared<const ErrorMessage>(
            "Internal error: client queue not found"));
    return;
  }

  Player *player = playerRepo.get(clientId);

  if (!player)
  {
    lobbyMonitor.sendTo(
        clientId,
        std::make_shared<const ErrorMessage>(
            "Must create a character before joining"));
    return;
  }

  if (!gameManager.joinGame(gameId, clientId, *clientQueue))
  {
    lobbyMonitor.sendTo(
        clientId,
        std::make_shared<const ErrorMessage>("Game not found or full"));
    return;
  }

  // Armamos DTO antes de mover el Player.
  PlayerDto playerDto = buildPlayerDto(*player);

  // Ahora sí, el jugador entra al GameWorld.
  gameManager.addPlayerToGame(gameId, std::move(*player));
  playerRepo.remove(clientId);

  std::string gameName;

  for (const auto &info : gameManager.listGames())
  {
    if (info.gameId == gameId)
    {
      gameName = info.gameName;
      break;
    }
  }

  // Primero avisamos al cliente que ya entró.
  // Esto evita que siga operando como lobby mientras el server ya lo trata como game.
  clientQueue->try_push(
      std::make_shared<const JoinOkMessage>(
          gameId,
          gameName,
          std::move(playerDto)));

  // Después redirigimos los mensajes entrantes del cliente a la cola del juego.
  auto *receiver = receiverRegistry.get(clientId);

  if (receiver)
  {
    receiver->setQueue(gameManager.getGameQueue(gameId));
  }

  // Ya no debe recibir mensajes desde el monitor del lobby.
  lobbyMonitor.removeQueue(clientId);

  // Finalmente sincronizamos inventario, jugadores existentes y spawn.
  gameManager.syncPlayerJoin(gameId, clientId);
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

    std::string originMapPath = gameManager.getRoomMapPath(originId);

    event.clientQueue->try_push(std::make_shared<const MapChangedMessage>(originMapPath));

    gameManager.joinGame(originId, event.clientId, *event.clientQueue);

    event.player.setTilePos(event.spawnTileX, event.spawnTileY);
    gameManager.addPlayerToGame(originId, std::move(event.player));

    auto *receiver = receiverRegistry.get(event.clientId);
    if (receiver)
      receiver->setQueue(gameManager.getGameQueue(originId));

    gameManager.syncPlayerJoin(originId, event.clientId);
  }
  else
  {
    std::string fullMapPath = event.targetMap;
    if (fullMapPath.find("assets/") == std::string::npos)
    {
      fullMapPath = "assets/sprites/MapAssets/" + fullMapPath + ".argmap";
    }

    uint32_t instanceId = gameManager.getOrCreateInstance(fullMapPath, event.fromRoomId);

    std::cout << "[SERVER] Enviando MSG_MAP_CHANGED al cliente para el mapa: " << fullMapPath << std::endl;
    event.clientQueue->try_push(std::make_shared<const MapChangedMessage>(fullMapPath));

    gameManager.joinGame(instanceId, event.clientId, *event.clientQueue);

    event.player.setTilePos(event.spawnTileX, event.spawnTileY);
    gameManager.addPlayerToGame(instanceId, std::move(event.player));

    auto *receiver = receiverRegistry.get(event.clientId);
    if (receiver)
      receiver->setQueue(gameManager.getGameQueue(instanceId));

    gameManager.syncPlayerJoin(instanceId, event.clientId);
  }
}

PlayerDto LobbyHandler::buildPlayerDto(const Player &player) const
{
  PlayerDto dto{};

  // Identidad del jugador.
  dto.playerID = static_cast<uint8_t>(player.getClientId());

  // Nombre del personaje.
  dto.nombre = player.getName();

  // Raza y clase.
  // Si estos campos no existen como .name, abajo te digo cómo resolverlo.
  dto.raza = player.getRace().name;
  std::cout << "[SERVER DTO] raza='" << dto.raza << "'" << std::endl;
  dto.clase = player.getCls().name;
  std::cout << "[SERVER DTO] clase='" << dto.clase << "'" << std::endl;

  // Apariencia inicial.
  dto.headId = 0;

  // Progresión.
  dto.level = player.getLevel();

  // Vida y maná.
  dto.hp = player.getHp();
  dto.hpMax = player.getMaxHp();
  dto.mana = player.getMana();
  dto.manaMax = player.getMaxMana();

  // Economía.
  dto.oro = static_cast<int>(player.getGold());
  dto.oroMax = 0;

  // Posición.
  dto.xpos = static_cast<uint16_t>(player.getPixelX());
  dto.ypos = static_cast<uint16_t>(player.getPixelY());

  // Experiencia.
  dto.exp = static_cast<int>(player.getExp());
  dto.expMax = 1000;

  // Estado lógico.
  dto.esFantasma = player.isGhost();

  // Atributos.
  dto.fuerza = player.getStrength();
  dto.agilidad = player.getAgility();
  dto.inteligencia = 10;
  dto.constitucion = 10;

  return dto;
}
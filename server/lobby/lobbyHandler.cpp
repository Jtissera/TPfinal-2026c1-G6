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
  try
  {
    while (true)
    {
      ClientMessage incoming;
      while (lobbyQueue.try_pop(incoming))
      {
        uint8_t opcode = incoming.message->opCode();
        auto it = handlers.find(opcode);
        if (it != handlers.end())
          it->second(incoming.clientId, *incoming.message);
        else
          lobbyMonitor.sendTo(
              incoming.clientId,
              std::make_shared<const ErrorMessage>("Not in a game"));
      }

      std::shared_ptr<LeaveEvent> leaveEvent;
      while (leaveQueue.try_pop(leaveEvent))
        handleLeaveGame(*leaveEvent);

      std::shared_ptr<InstanceTransitionEvent> transition;
      while (transitionQueue.try_pop(transition))
        handleInstanceTransition(*transition);

      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  }
  catch (const ClosedQueue &)
  {
  }
  catch (const std::exception &e)
  {
    std::cerr << "[LobbyHandler] error: " << e.what() << std::endl;
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
                                         msg.getClase(), 6, 7);
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
  uint32_t gameId = joinMsg.getGameId();

  Queue<std::shared_ptr<const Message>> *clientQueue =
      lobbyMonitor.getQueue(clientId);
  if (!clientQueue)
  {
    lobbyMonitor.sendTo(clientId,
                        std::make_shared<const ErrorMessage>(
                            "Internal error: client queue not found"));
    return;
  }

  Player *player = playerRepo.get(clientId);
  if (!player)
  {
    lobbyMonitor.sendTo(clientId,
                        std::make_shared<const ErrorMessage>(
                            "Must create a character before joining"));
    return;
  }

  if (!gameManager.joinGame(gameId, clientId, *clientQueue))
  {
    lobbyMonitor.sendTo(clientId, std::make_shared<const ErrorMessage>(
                                      "Game not found or full"));
    return;
  }

  gameManager.addPlayerToGame(gameId, std::move(*player));
  playerRepo.remove(clientId);

  auto *receiver = receiverRegistry.get(clientId);
  if (receiver)
    receiver->setQueue(gameManager.getGameQueue(gameId));

  lobbyMonitor.removeQueue(clientId);

  std::string gameName;
  for (const auto &info : gameManager.listGames())
    if (info.gameId == gameId)
    {
      gameName = info.gameName;
      break;
    }

  clientQueue->try_push(
      std::make_shared<const JoinOkMessage>(gameId, gameName));
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
    // EXIT tile: volver al room de origen
    uint32_t originId = gameManager.getOriginRoomId(event.fromRoomId);
    if (originId == 0)
      return;

    gameManager.joinGame(originId, event.clientId, *event.clientQueue);
    event.player.setTilePos(event.spawnTileX, event.spawnTileY);
    gameManager.addPlayerToGame(originId, std::move(event.player));

    auto *receiver = receiverRegistry.get(event.clientId);
    if (receiver)
      receiver->setQueue(gameManager.getGameQueue(originId));
  }
  else
  {
    // Entrada: ir al room de instancia (crearlo si no existe)
    uint32_t instanceId = gameManager.getOrCreateInstance(
        event.targetMap, event.fromRoomId);

    gameManager.joinGame(instanceId, event.clientId, *event.clientQueue);
    gameManager.addPlayerToGame(instanceId, std::move(event.player));

    auto *receiver = receiverRegistry.get(event.clientId);
    if (receiver)
      receiver->setQueue(gameManager.getGameQueue(instanceId));
  }
}
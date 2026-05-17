#include "lobbyHandler.h"

#include "../common/network/messages/client/lobby/listGamesMessage.h"
#include "../common/network/messages/client/lobby/createGameMessage.h"
#include "../common/network/messages/client/lobby/joinGameMessage.h"

LobbyHandler::LobbyHandler(Queue<ClientMessage> &lobbyQueue,
                           Monitor &lobbyMonitor,
                           GameManager &gameManager,
                           ClientRegistry &clientRegistry,
                           ReceiverRegistry &receiverRegistry)
    : lobbyQueue(lobbyQueue),
      lobbyMonitor(lobbyMonitor),
      gameManager(gameManager),
      clientRegistry(clientRegistry),
      receiverRegistry(receiverRegistry) {}

void LobbyHandler::run()
{
    try
    {
        while (true)
        {
            ClientMessage incoming = lobbyQueue.pop();

            uint8_t opcode = incoming.message->opCode();

            std::cout << "[LobbyHandler] client=" << incoming.clientId
                      << " opcode=0x" << std::hex << static_cast<int>(opcode)
                      << std::dec << std::endl;

            if (opcode == static_cast<uint8_t>(ClientOpCode::MSG_CONNECT))
            {
                handleConnect(incoming.clientId, *incoming.message);
            }
            else if (opcode == static_cast<uint8_t>(ClientOpCode::MSG_LIST_GAMES))
            {
                handleListGames(incoming.clientId);
            }
            else if (opcode == static_cast<uint8_t>(ClientOpCode::MSG_CREATE_GAME))
            {
                handleCreateGame(incoming.clientId, *incoming.message);
            }
            else if (opcode == static_cast<uint8_t>(ClientOpCode::MSG_JOIN_GAME))
            {
                handleJoinGame(incoming.clientId, *incoming.message);
            }
            else
            {
                auto error = std::make_shared<const ErrorMessage>("Not in a game");
                lobbyMonitor.sendTo(incoming.clientId, error);
            }
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
}

void LobbyHandler::handleConnect(uint32_t clientId, const Message &message)
{
    const auto &connectMsg = static_cast<const ConnectMessage &>(message);

    std::cout << "[LobbyHandler] client=" << clientId
              << " username=" << connectMsg.getUsername()
              << " connected" << std::endl;

    auto response = std::make_shared<const ConnectOkMessage>();
    lobbyMonitor.sendTo(clientId, response);
}

void LobbyHandler::handleListGames(uint32_t clientId)
{
    auto games = gameManager.listGames();
    auto response = std::make_shared<const GameListMessage>(std::move(games));
    lobbyMonitor.sendTo(clientId, response);
}

void LobbyHandler::handleCreateGame(uint32_t clientId, const Message &message)
{
    const auto &createMsg = static_cast<const CreateGameMessage &>(message);
    uint32_t gameId = gameManager.createGame(createMsg.getGameName(), createMsg.getMaxPlayers());

    auto response = std::make_shared<const GameCreatedMessage>(gameId, createMsg.getGameName(), createMsg.getMaxPlayers());
    lobbyMonitor.sendTo(clientId, response);
}

void LobbyHandler::handleJoinGame(uint32_t clientId, const Message &message)
{
    const auto &joinMsg = static_cast<const JoinGameMessage &>(message);
    uint32_t gameId = joinMsg.getGameId();

    auto *clientQueue = clientRegistry.get(clientId);
    if (!clientQueue)
    {
        auto error = std::make_shared<const ErrorMessage>("Internal error: client queue not found");
        lobbyMonitor.sendTo(clientId, error);
        return;
    }

    std::string gameName;
    for (const auto &info : gameManager.listGames())
    {
        if (info.gameId == gameId)
        {
            gameName = info.gameName;
            break;
        }
    }

    if (!gameManager.joinGame(gameId, clientId, *clientQueue))
    {
        auto error = std::make_shared<const ErrorMessage>("Game not found or full");
        lobbyMonitor.sendTo(clientId, error);
        return;
    }

    // Swap: el receiver deja de escribir en lobbyQueue
    // y empieza a escribir en la gameQueue de su sala
    auto *receiver = receiverRegistry.get(clientId);
    if (receiver)
        receiver->setQueue(gameManager.getGameQueue(gameId));

    // Una vez en la sala, sacar al cliente del lobbyMonitor
    // para que los mensajes de juego no se mezclen con el lobby
    lobbyMonitor.removeQueue(clientId);

    auto response = std::make_shared<const JoinOkMessage>(gameId, gameName);
    clientQueue->try_push(response);
}
#include "lobbyHandler.h"

#include "../common/network/messages/client/lobby/listGamesMessage.h"
#include "../common/network/messages/client/lobby/createGameMessage.h"
#include "../common/network/messages/client/lobby/joinGameMessage.h"

LobbyHandler::LobbyHandler(Queue<ClientMessage>& lobbyQueue,
                           Monitor& lobbyMonitor,
                           GameManager& gameManager,
                           ClientRegistry& clientRegistry,
                           ReceiverRegistry& receiverRegistry,
                           PlayerRepository& playerRepo,
                           PlayerFactory& playerFactory)
    : lobbyQueue(lobbyQueue),
      lobbyMonitor(lobbyMonitor),
      gameManager(gameManager),
      clientRegistry(clientRegistry),
      receiverRegistry(receiverRegistry),
      playerRepo(playerRepo),
      playerFactory(playerFactory) {}

void LobbyHandler::run() {
    try {
        while (true) {
            ClientMessage incoming = lobbyQueue.pop();
            uint8_t opcode = incoming.message->opCode();

            std::cout << "[LobbyHandler] client=" << incoming.clientId
                      << " opcode=0x" << std::hex << static_cast<int>(opcode)
                      << std::dec << std::endl;

            if (opcode == static_cast<uint8_t>(ClientOpCode::MSG_CONNECT)) {
                handleConnect(incoming.clientId, *incoming.message);
            } else if (opcode == static_cast<uint8_t>(ClientOpCode::MSG_CREATE_CHAR)) {
                handleCreateChar(incoming.clientId, *incoming.message);
            } else if (opcode == static_cast<uint8_t>(ClientOpCode::MSG_LIST_GAMES)) {
                handleListGames(incoming.clientId);
            } else if (opcode == static_cast<uint8_t>(ClientOpCode::MSG_CREATE_GAME)) {
                handleCreateGame(incoming.clientId, *incoming.message);
            } else if (opcode == static_cast<uint8_t>(ClientOpCode::MSG_JOIN_GAME)) {
                handleJoinGame(incoming.clientId, *incoming.message);
            } else {
                auto error = std::make_shared<const ErrorMessage>("Not in a game");
                lobbyMonitor.sendTo(incoming.clientId, error);
            }
        }
    }
    catch (const ClosedQueue&) {}
    catch (const std::exception& e) {
        std::cerr << "[LobbyHandler] error: " << e.what() << std::endl;
    }
}

void LobbyHandler::stop() {
    Thread::stop();
    lobbyQueue.close();
}

void LobbyHandler::handleConnect(uint32_t clientId, const Message& message) {
    const auto& connectMsg = static_cast<const ConnectMessage&>(message);
    std::cout << "[LobbyHandler] client=" << clientId
              << " username=" << connectMsg.getUsername()
              << " connected" << std::endl;

    auto response = std::make_shared<const ConnectOkMessage>();
    lobbyMonitor.sendTo(clientId, response);
}

void LobbyHandler::handleCreateChar(uint32_t clientId, const Message& message) {
    const auto& msg = static_cast<const CreateCharMessage&>(message);
    try {
        Player player = playerFactory.create(
            clientId,
            msg.getName(),
            msg.getRaza(),
            msg.getClase(),
            6 * 96, 7 * 96
        );
        playerRepo.save(clientId, std::move(player));
        auto response = std::make_shared<const CreateOkMessage>();
        lobbyMonitor.sendTo(clientId, response);
    } catch (const std::exception& e) {
        auto error = std::make_shared<const ErrorMessage>(e.what());
        lobbyMonitor.sendTo(clientId, error);
    }
}

void LobbyHandler::handleListGames(uint32_t clientId) {
    auto games = gameManager.listGames();
    auto response = std::make_shared<const GameListMessage>(std::move(games));
    lobbyMonitor.sendTo(clientId, response);
}

void LobbyHandler::handleCreateGame(uint32_t clientId, const Message& message) {
    const auto& createMsg = static_cast<const CreateGameMessage&>(message);
    uint32_t gameId = gameManager.createGame(createMsg.getGameName(),
                                             createMsg.getMaxPlayers());
    auto response = std::make_shared<const GameCreatedMessage>(
        gameId, createMsg.getGameName(), createMsg.getMaxPlayers());
    lobbyMonitor.sendTo(clientId, response);
}

void LobbyHandler::handleJoinGame(uint32_t clientId, const Message& message) {
    const auto& joinMsg = static_cast<const JoinGameMessage&>(message);
    uint32_t gameId = joinMsg.getGameId();

    auto* clientQueue = clientRegistry.get(clientId);
    if (!clientQueue) {
        lobbyMonitor.sendTo(clientId,
            std::make_shared<const ErrorMessage>("Internal error: client queue not found"));
        return;
    }

    Player* player = playerRepo.get(clientId);
    if (!player) {
        lobbyMonitor.sendTo(clientId,
            std::make_shared<const ErrorMessage>("Must create a character before joining"));
        return;
    }

    std::string gameName;
    for (const auto& info : gameManager.listGames()) {
        if (info.gameId == gameId) { gameName = info.gameName; break; }
    }

    if (!gameManager.joinGame(gameId, clientId, *clientQueue, std::move(*player))) {
        lobbyMonitor.sendTo(clientId,
            std::make_shared<const ErrorMessage>("Game not found or full"));
        return;
    }

    playerRepo.remove(clientId);  // ya está en GameWorld, no lo necesitamos más acá

    auto* receiver = receiverRegistry.get(clientId);
    if (receiver)
        receiver->setQueue(gameManager.getGameQueue(gameId));

    lobbyMonitor.removeQueue(clientId);

    auto response = std::make_shared<const JoinOkMessage>(gameId, gameName);
    clientQueue->try_push(response);
}
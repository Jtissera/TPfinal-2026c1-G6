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
                lobbyMonitor.sendTo(incoming.clientId,
                    std::make_shared<const ErrorMessage>("Not in a game"));
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

    lobbyMonitor.sendTo(clientId, std::make_shared<const ConnectOkMessage>());
}

void LobbyHandler::handleCreateChar(uint32_t clientId, const Message& message) {
    const auto& msg = static_cast<const CreateCharMessage&>(message);
    try {
        Player player = playerFactory.create(    
        clientId,
        msg.getName(),
        msg.getRaza(),
        msg.getClase(),
        6, 7  // tile 6,7 -> cte
    );
        playerRepo.save(clientId, std::move(player));
        lobbyMonitor.sendTo(clientId, std::make_shared<const CreateOkMessage>());
    } catch (const std::exception& e) {
        lobbyMonitor.sendTo(clientId,
            std::make_shared<const ErrorMessage>(e.what()));
    }
}

void LobbyHandler::handleListGames(uint32_t clientId) {
    auto games = gameManager.listGames();
    lobbyMonitor.sendTo(clientId,
        std::make_shared<const GameListMessage>(std::move(games)));
}

void LobbyHandler::handleCreateGame(uint32_t clientId, const Message& message) {
    const auto& createMsg = static_cast<const CreateGameMessage&>(message);
    uint32_t gameId = gameManager.createGame(createMsg.getGameName(),
                                             createMsg.getMaxPlayers());
    lobbyMonitor.sendTo(clientId,
        std::make_shared<const GameCreatedMessage>(
            gameId, createMsg.getGameName(), createMsg.getMaxPlayers()));
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

    if (!gameManager.joinGame(gameId, clientId, *clientQueue)) {
        lobbyMonitor.sendTo(clientId,
            std::make_shared<const ErrorMessage>("Game not found or full"));
        return;
    }
    PlayerDto playerDto = buildPlayerDto(*player);
    gameManager.addPlayerToGame(gameId, std::move(*player));
    playerRepo.remove(clientId);

    auto* receiver = receiverRegistry.get(clientId);
    std::cout << "[LobbyHandler] receiver for client=" << clientId 
          << " is " << (receiver ? "found" : "NULL") << std::endl;
    if (receiver)
        receiver->setQueue(gameManager.getGameQueue(gameId));

    lobbyMonitor.removeQueue(clientId);

    std::string gameName;
    for (const auto& info : gameManager.listGames())
        if (info.gameId == gameId) { gameName = info.gameName; break; }

    clientQueue->try_push(std::make_shared<const JoinOkMessage>(gameId, gameName,std::move(playerDto)));
}

PlayerDto LobbyHandler::buildPlayerDto(const Player& player) const {
    PlayerDto dto{};

    // Identidad del jugador.
    dto.playerID = static_cast<uint8_t>(player.getClientId());

    // Nombre del personaje.
    dto.nombre = player.getName();

    // Raza y clase.
    // Si estos campos no existen como .name, abajo te digo cómo resolverlo.
    dto.raza = player.getRace().name;
    dto.clase = player.getCls().name;

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
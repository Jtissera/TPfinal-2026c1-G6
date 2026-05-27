#pragma once

#include "../common/network/sockets.h"
#include "../common/queue.h"
#include "clientMessage.h"
#include "monitorQueues.h"
#include "gameManager.h"
#include "lobbyHandler.h"
#include "playerRepository.h"
#include "game/playerFactory.h"
#include "acceptor.h"
#include "receiverRegistry.h"
#include "game/classRepository.h"
#include "game/raceRepository.h"
#include <toml++/toml.h>

class Server {
public:
    explicit Server(const char* servname);
    int run();

private:
    ClassRepository  classRepo;
    RaceRepository   raceRepo;
    PlayerFactory    playerFactory;
    PlayerRepository playerRepo;

    Monitor              lobbyMonitor;
    Queue<ClientMessage> lobbyQueue;

    ClientRegistry   clientRegistry;
    ReceiverRegistry receiverRegistry;
    GameManager      gameManager;
    LobbyHandler     lobbyHandler;

    Socket   socket;
    Acceptor acceptor;
};
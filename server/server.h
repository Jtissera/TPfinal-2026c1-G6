#pragma once
#include "../common/network/sockets.h"
#include "../common/queue.h"
#include "clientMessage.h"
#include "monitorQueues.h"
#include "gameManager.h"
#include "lobbyHandler.h"
#include "charHandler.h"
#include "playerRepository.h"
#include "game/playerFactory.h"
#include "acceptor.h"
#include "receiverRegistry.h"
#include "game/classRepository.h"
#include "game/raceRepository.h"
#include <toml++/toml.h>

#include <iostream>

class Server {
public:
    explicit Server(const char* servname);
    int run();
private:
    ClassRepository classRepo;
    RaceRepository  raceRepo;
    PlayerFactory   playerFactory;
    PlayerRepository playerRepo;

    Monitor lobbyMonitor;
    Queue<ClientMessage> lobbyQueue;

    Monitor charMonitor;
    Queue<ClientMessage> charQueue;

    ClientRegistry clientRegistry;
    ReceiverRegistry receiverRegistry;
    GameManager gameManager;

    LobbyHandler lobbyHandler;
    CharHandler  charHandler;

    Socket socket;
    Acceptor acceptor;
};
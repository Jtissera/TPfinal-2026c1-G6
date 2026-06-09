
#ifndef TALLER_TP_GAMECLIENT_H
#define TALLER_TP_GAMECLIENT_H
#pragma once
#include <memory>

#include "ClientReceiver.h"
#include "ClientSender.h"
#include "common/queue.h"
#include "common/network/messages/message.h"
#include "common/dtos/gameTypes.h"
#include "network/clientProtocolFactory.h"
#include "../common/network/sockets.h"
#include "Game.h"

class GameClient {

public:
    GameClient(Socket& socket,uint32_t idPlayer,const PlayerDto& playerDto,SDL_Window* window,SDL_Renderer* renderer);
    void run();

private:
    Socket& socket;
    const uint32_t idPlayer;
    const PlayerDto playerDto;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    ClientProtocolFactory factory;

    Protocol senderProtocol;
    Protocol receiverProtocol;

    Queue<std::shared_ptr<const Message>> sendQueue;
    Queue<std::shared_ptr<const Message>> receiveQueue;

    ClientSender sender;
    ClientReceiver receiver;

    Game gameLoop;

};

#endif //TALLER_TP_GAMECLIENT_H

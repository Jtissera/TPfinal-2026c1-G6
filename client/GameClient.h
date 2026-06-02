#pragma once
#include <memory>

#include "ClientReceiver.h"
#include "ClientSender.h"
#include "common/queue.h"
#include "common/network/messages/message.h"
#include "common/dtos/gameTypes.h"
#include "network/clientProtocolFactory.h"
#include "Game.h"

class GameClient {
public:
    GameClient(Socket& socket, uint32_t idPlayer, const PlayerDto& playerDto);
    void run();

private:
    uint32_t idPlayer;
    PlayerDto playerDto;
    ClientProtocolFactory factory;
    Protocol protocol;
    Queue<std::shared_ptr<const Message>> sendQueue;
    Queue<std::shared_ptr<const Message>> receiveQueue;
    ClientSender sender;
    ClientReceiver receiver;
    Game gameLoop;
};
#pragma once

#include <iostream>
#include <string>

#include "../common/liberror.h"
#include "../common/network/sockets.h"
#include "../common/network/messages/client/auth/connectMessage.h"
#include "../common/network/messages/client/lobby/listGamesMessage.h"
#include "../common/network/messages/client/lobby/createGameMessage.h"
#include "../common/network/messages/client/lobby/joinGameMessage.h"

#include "../common/network/messages/server/lobby/gameCreatedMessage.h"
#include "../common/network/messages/server/lobby/gameListMessage.h"
#include "../common/network/messages/server/lobby/joinOkMessage.h"
#include "../common/network/messages/server/error/errorMessage.h"

#include "network/clientProtocolFactory.h"

#include <SDL2/SDL.h>

class Client {
public:
    Client(const char* hostname, const char* servname,
           SDL_Renderer* renderer, int windowW, int windowH);

    int run();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

private:
    Socket                socket;
    ClientProtocolFactory factory;
    Protocol              protocol;

    SDL_Renderer* renderer;
    int windowW;
    int windowH;

    static constexpr uint8_t    PROTOCOL_VERSION = 0x01;
    static constexpr const char* FONT_PATH = "assets/sprites/MapAssets/arial.ttf";
};

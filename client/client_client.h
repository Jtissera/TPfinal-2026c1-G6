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

class Client
{
public:
    Client(const char *hostname, const char *servname);

    int run();

    Client(const Client &) = delete;
    Client &operator=(const Client &) = delete;

private:
    Socket socket;
    ClientProtocolFactory factory;
    Protocol protocol;
};

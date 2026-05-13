#pragma once

#include <iostream>
#include <string>

#include "../common/liberror.h"
#include "../common/network/sockets.h"

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

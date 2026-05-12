#ifndef CLIENT_CLIENT_H_
#define CLIENT_CLIENT_H_

#include <iostream>
#include <string>

#include "../common/common_liberror.h"
#include "../common/common_protocol.h"
#include "../common/common_socket.h"

// Cliente minimo
class Client {
private:
    Socket skt;
    Protocol protocol;

public:
    Client(const char* hostname, const char* servname);

    int run();
};

#endif
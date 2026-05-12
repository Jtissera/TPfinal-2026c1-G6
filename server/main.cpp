#include <iostream>
#include <exception>
#include <cstddef>

#include "server_server.h"

int main(int argc, char* argv[]) {
    std::cout << "[Server] Argentum Online server starting..." << std::endl;
    std::cout << "[Server] Running. Press Ctrl+C to stop." << std::endl;
  
    int ret = 1;

    char* servname = NULL;

    if (argc == 2) {
        servname = argv[1];

    } else {
        std::cerr << "Bad program call. Expected " << argv[0] << " <servname>\n";
        return ret;
    }

    Server server(servname);
    server.run();

    ret = 0;
    return ret;
}




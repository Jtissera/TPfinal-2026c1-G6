#include <cstddef>
#include <exception>
#include <iostream>

#include "server.h"

int main(int argc, char *argv[]) {
  std::cout << "[Server] Argentum Online server starting..." << std::endl;
  std::cout << "[Server] Running. Press Q to stop." << std::endl;

  const char *servname = NULL;

  if (argc == 2) {
    servname = argv[1];
  } else {
    std::cerr << "Bad program call. Expected " << argv[0] << " <servname>\n";
    return 1;
  }

  Server server(servname);
  server.run();

  ;
  return 0;
}

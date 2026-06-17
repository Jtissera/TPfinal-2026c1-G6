#include "client_client.h"

#include <iostream>
#include <stdexcept>

#include "../common/liberror.h"
#include "../common/network/messages/client/auth/connectMessage.h"
#include "../common/network/messages/client/auth/createCharMessage.h"
#include "../common/network/messages/client/auth/loginMessage.h"
#include "../common/network/messages/client/lobby/createGameMessage.h"
#include "../common/network/messages/client/lobby/joinGameMessage.h"
#include "../common/network/messages/client/lobby/listGamesMessage.h"
#include "../common/network/messages/server/auth/loginOkMessage.h"
#include "../common/network/messages/server/error/errorMessage.h"
#include "../common/network/messages/server/lobby/gameCreatedMessage.h"
#include "../common/network/messages/server/lobby/gameListMessage.h"
#include "../common/network/messages/server/lobby/joinOkMessage.h"
#include "../common/network/protocol/serverOpCode.h"
#include "../common/network/sockets.h"
#include "network/clientProtocolFactory.h"

#include "Game.h"
#include "GameClient.h"
#include "sdl/screens/ConfigScreen.h"
#include "sdl/screens/CreateCharScreen.h"
#include "sdl/screens/MainMenuScreen.h"
#include "sdl/screens/PlaceholderLobbyScreen.h"
#include "sdl/screens/Screen.h"

static void flushSDLEvents() {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
  }
}

Client::Client(const char *hostname, const char *servname,
               SDL_Renderer *renderer, SDL_Window *window, int windowW,
               int windowH)
    : hostname(hostname), servname(servname), renderer(renderer),
      window(window), windowW(windowW), windowH(windowH),
      config(ClientConfig::load()),
      serverWatcher(hostname, servname, serverShutdownDetected) {}

Client::~Client() {
  if (serverWatcher.is_alive()) {
    serverWatcher.stop();
    serverWatcher.join();
  }
}

int Client::run() {
  serverWatcher.start();

  std::string pendingError;

  auto detenerWatcher = [this]() {
    serverWatcher.stop();
    serverWatcher.join();
  };

  while (true) {
    flushSDLEvents();

    // ---------------------- 1. Menu principal ----------------------
    ScreenResult menuResult;
    {
      MainMenuScreen menu(renderer, windowW, windowH, FONT_PATH);
      if (!pendingError.empty()) {
        menu.setError(pendingError);
        pendingError.clear();
      }
      menuResult = menu.run();
    }

    if (menuResult == ScreenResult::QUIT) {
      detenerWatcher();
      return 0;
    }

    // ---------------------- AR-80: Configuracion ----------------------
    if (menuResult == ScreenResult::GO_CONFIG) {
      flushSDLEvents();
      ConfigScreen cfg(renderer, window, windowW, windowH, FONT_PATH, config);
      ScreenResult cfgResult = cfg.run();
      if (cfgResult == ScreenResult::QUIT) {
        detenerWatcher();
        return 0;
      }
      continue;
    }

    // ---------------------- 2. Pantalla crear personaje/login
    // ----------------------
    bool isCreate = (menuResult == ScreenResult::GO_CREATE_CHAR);

    std::string username;
    std::string raza;
    std::string clase;

    {
      flushSDLEvents();
      auto mode = isCreate ? CreateCharScreen::Mode::CREATE
                           : CreateCharScreen::Mode::LOGIN;
      CreateCharScreen charScreen(renderer, windowW, windowH, FONT_PATH, mode);
      ScreenResult charResult = charScreen.run();

      if (charResult == ScreenResult::QUIT) {
        std::cout << "[Client] Cerrando aplicación..." << std::endl;
        detenerWatcher();
        return 0;
      }
      if (charResult == ScreenResult::GO_MAIN_MENU)
        continue;

      username = charScreen.getUsername();
      raza = charScreen.getRaza();
      clase = charScreen.getClase();
    }

    // ---------------------- 3. Conectar al servidor ----------------------
    try {
      Socket socket(hostname.c_str(), servname.c_str());
      ClientProtocolFactory factory;
      Protocol protocol = factory.createProtocol(socket);

      protocol.send(ConnectMessage(PROTOCOL_VERSION, username));

      auto connectResponse = protocol.receive();
      if (connectResponse->opCode() !=
          static_cast<uint8_t>(ServerOpCode::MSG_CONNECT_OK)) {
        pendingError = "Error: el servidor rechazo la conexion.";
        continue;
      }

      if (isCreate) {
        protocol.send(CreateCharMessage(username, raza, clase));
        auto createResponse = protocol.receive();
        if (createResponse->opCode() ==
            static_cast<uint8_t>(ServerOpCode::MSG_ERROR)) {
          const auto &err = static_cast<const ErrorMessage &>(*createResponse);
          pendingError = err.getReason();
          continue;
        }
      } else {
        protocol.send(LoginMessage(username));
        auto loginResponse = protocol.receive();
        if (loginResponse->opCode() ==
            static_cast<uint8_t>(ServerOpCode::MSG_ERROR)) {
          const auto &err = static_cast<const ErrorMessage &>(*loginResponse);
          pendingError = err.getReason();
          continue;
        }
      }

      // ---------------------- 4. Lobby ----------------------
      {
        flushSDLEvents();
        PlaceholderLobbyScreen lobby(renderer, windowW, windowH, FONT_PATH,
                                     protocol, username);
        ScreenResult lobbyResult = lobby.run();

        if (lobbyResult == ScreenResult::QUIT) {
          detenerWatcher();
          return 0;
        }
        if (lobbyResult == ScreenResult::GO_MAIN_MENU)
          continue;

        if (lobbyResult == ScreenResult::GO_LOBBY) {
          PlayerDto playerDto = lobby.getJoinedPlayerDto();
          std::string mapPath = lobby.getChosenMapPath();
          auto pending = lobby.takePendingMessage(); 

          GameClient gameClient(socket, playerDto.playerID, playerDto, window,
                                renderer, mapPath, std::move(pending));
          gameClient.run();

          if (gameClient.wasDisconnectedByServer() || serverShutdownDetected) {
            std::cerr << "[Client] Servidor cerró conexión." << std::endl;
            detenerWatcher();
            return 0;
          }
        }
      }

      continue;
    } catch (const ClosedSocket &) {
      std::cerr << "[Client] Conexión perdida, cerrando cliente." << std::endl;
      detenerWatcher();
      return 0;
    } catch (const std::exception &e) {
      pendingError = std::string("Error fatal: ") + e.what();
      std::cerr << pendingError << std::endl;
      detenerWatcher();
      return 1;
    }
  }
}
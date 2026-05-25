#include "client_client.h"

#include <iostream>
#include <stdexcept>

#include "../common/liberror.h"
#include "../common/network/sockets.h"
#include "../common/network/messages/client/auth/connectMessage.h"
#include "../common/network/messages/client/auth/createCharMessage.h"
#include "../common/network/messages/client/lobby/listGamesMessage.h"
#include "../common/network/messages/client/lobby/createGameMessage.h"
#include "../common/network/messages/client/lobby/joinGameMessage.h"
#include "../common/network/messages/server/lobby/gameCreatedMessage.h"
#include "../common/network/messages/server/lobby/gameListMessage.h"
#include "../common/network/messages/server/lobby/joinOkMessage.h"
#include "../common/network/messages/server/error/errorMessage.h"
#include "../common/network/protocol/serverOpCode.h"
#include "network/clientProtocolFactory.h"

#include "Game.h"
#include "GameClient.h"
#include "sdl/screens/Screen.h"
#include "sdl/screens/MainMenuScreen.h"
#include "sdl/screens/CreateCharScreen.h"
#include "sdl/screens/PlaceholderLobbyScreen.h"
#include "sdl/screens/ConfigScreen.h"

static void flushSDLEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {}
}

Client::Client(const char* hostname, const char* servname,
               SDL_Renderer* renderer, SDL_Window* window,
               int windowW, int windowH)
    : hostname(hostname), servname(servname),
      renderer(renderer), window(window),
      windowW(windowW), windowH(windowH),
      config(ClientConfig::load())       // carga ~/.config/argentum/client.toml
{}

int Client::run()
{
    std::string pendingError;

    while (true)
    {
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

        if (menuResult == ScreenResult::QUIT) return 0;

        // ---------------------- AR-80: Configuracion ----------------------
        if (menuResult == ScreenResult::GO_CONFIG) {
            flushSDLEvents();
            ConfigScreen cfg(renderer, window, windowW, windowH, FONT_PATH, config);
            ScreenResult cfgResult = cfg.run();
            if (cfgResult == ScreenResult::QUIT) return 0;
            continue;  // vuelve al menú
        }

        // ---------------------- 2. Pantalla crear personaje/login ----------------------
        bool isCreate = (menuResult == ScreenResult::GO_CREATE_CHAR);

        std::string username;
        Raza  raza  = Raza::HUMANO;
        Clase clase = Clase::MAGO;

        {
            flushSDLEvents();
            auto mode = isCreate ? CreateCharScreen::Mode::CREATE
                                 : CreateCharScreen::Mode::LOGIN;
            CreateCharScreen charScreen(renderer, windowW, windowH, FONT_PATH, mode);
            ScreenResult charResult = charScreen.run();

            if (charResult == ScreenResult::QUIT)         return 0;
            if (charResult == ScreenResult::GO_MAIN_MENU) continue;

            username = charScreen.getUsername();
            raza     = charScreen.getRaza();
            clase    = charScreen.getClase();
        }

        // ---------------------- 3. Conectar al servidor ----------------------
        try
        {
            Socket socket(hostname.c_str(), servname.c_str());
            ClientProtocolFactory factory;
            Protocol protocol = factory.createProtocol(socket);

            protocol.send(ConnectMessage(PROTOCOL_VERSION, username));

            auto connectResponse = protocol.receive();
            if (connectResponse->opCode() != static_cast<uint8_t>(ServerOpCode::MSG_CONNECT_OK))
            {
                pendingError = "Error: el servidor rechazo la conexion.";
                continue;
            }

            if (isCreate) {
                protocol.send(CreateCharMessage(username, raza, clase));
                auto createResponse = protocol.receive();
                if (createResponse->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_ERROR)) {
                    const auto& err = static_cast<const ErrorMessage&>(*createResponse);
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

                if (lobbyResult == ScreenResult::QUIT)         return 0;
                if (lobbyResult == ScreenResult::GO_MAIN_MENU) continue;

                if (lobbyResult == ScreenResult::GO_LOBBY) {
                    PlayerDto playerDto;
                    playerDto.nombre  = username;
                    playerDto.raza    = raza;
                    playerDto.clase   = clase;
                    playerDto.xpos    = 1500;
                    playerDto.ypos    = 1200;
                    playerDto.hp      = 100;
                    playerDto.hpMax   = 100;
                    playerDto.mana    = 100;
                    playerDto.manaMax = 100;
                    playerDto.level   = 1;
                    playerDto.oro     = 2000;

                    GameClient gameClient(socket, 1, playerDto);
                    gameClient.run();
                }
            }

            continue;
        }
        catch (const ClosedSocket&)
        {
            pendingError = "Conexion cerrada por el servidor. (Operacion no soportada todavia)";
            continue;
        }
        catch (const std::exception& e)
        {
            pendingError = std::string("Error: ") + e.what();
            continue;
        }
    }
}

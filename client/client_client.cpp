#include "client_client.h"

#include "../common/network/messages/client/auth/connectMessage.h"
#include "../common/network/protocol/clientOpCode.h"
#include "Game.h"
#include "GameClient.h"
#include "sdl/screens/Screen.h"
#include "sdl/screens/MainMenuScreen.h"

Client::Client(const char* hostname, const char* servname,
               SDL_Renderer* renderer, int windowW, int windowH)
    : socket(hostname, servname),
      factory(),
      protocol(factory.createProtocol(socket)),
      renderer(renderer),
      windowW(windowW),
      windowH(windowH)
{}

int Client::run()
{
    try
    {
        // ---------------- 1. Menu principal SDL ----------------

        MainMenuScreen menu(renderer, windowW, windowH, FONT_PATH);
        ScreenResult menuResult = menu.run();

        if (menuResult == ScreenResult::QUIT)
            return 0;

        if (menuResult == ScreenResult::GO_CONFIG) {
            // AR-80: pantalla de configuración (pendiente)
            return 0;
        }

        // ---------------- 2. Pedir nombre y conectar al servidor ----------------
        std::string username;
        if (menuResult == ScreenResult::GO_CREATE_CHAR) {
            std::cout << "Nombre del nuevo personaje: ";
        } else {
            std::cout << "Nombre de jugador: ";
        }
        std::getline(std::cin, username);

        protocol.send(ConnectMessage(PROTOCOL_VERSION, username));

        auto connectResponse = protocol.receive();
        if (connectResponse->opCode() != static_cast<uint8_t>(ServerOpCode::MSG_CONNECT_OK))
        {
            // Error de conexión: lo mostramos en pantalla SDL
            MainMenuScreen errMenu(renderer, windowW, windowH, FONT_PATH);
            errMenu.setError("Error: el servidor rechazo la conexion.");
            errMenu.run();
            return 1;
        }

        std::cout << "[Client] Conectado como " << username << "." << std::endl;

        // ---------------- 3. Lobby por terminal ----------------
        bool inGame = false;

        while (!inGame)
        {
            std::cout << "\nOpciones:\n"
                      << "  1) Listar partidas\n"
                      << "  2) Crear partida\n"
                      << "  3) Unirse a partida\n"
                      << "  q) Salir\n"
                      << "> ";

            std::string option;
            std::getline(std::cin, option);

            if (option == "q")
            {
                std::cout << "[Client] Saliendo." << std::endl;
                break;
            }
            else if (option == "1")
            {
                protocol.send(ListGamesMessage());

                auto response = protocol.receive();
                if (response->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_GAME_LIST))
                {
                    const auto& listMsg = static_cast<const GameListMessage&>(*response);
                    if (listMsg.getGames().empty())
                    {
                        std::cout << "  No hay partidas disponibles." << std::endl;
                    }
                    else
                    {
                        std::cout << "  Partidas disponibles:" << std::endl;
                        for (const auto& game : listMsg.getGames())
                        {
                            std::cout << "    id=" << game.gameId
                                      << " nombre=" << game.gameName
                                      << " jugadores=" << static_cast<int>(game.playerCount)
                                      << "/" << static_cast<int>(game.maxPlayers)
                                      << std::endl;
                        }
                    }
                }
            }
            else if (option == "2")
            {
                std::cout << "Nombre de partida: ";
                std::string gameName;
                std::getline(std::cin, gameName);

                std::cout << "Maximo de jugadores: ";
                std::string maxStr;
                std::getline(std::cin, maxStr);
                uint8_t maxPlayers = static_cast<uint8_t>(std::stoi(maxStr));

                protocol.send(CreateGameMessage(gameName, maxPlayers));

                auto response = protocol.receive();
                if (response->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_GAME_CREATED))
                {
                    const auto& created = static_cast<const GameCreatedMessage&>(*response);
                    protocol.send(JoinGameMessage(created.getGameId()));

                    auto joinResponse = protocol.receive();
                    if (joinResponse->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_JOIN_OK))
                    {
                        const auto& joinOk = static_cast<const JoinOkMessage&>(*joinResponse);
                        std::cout << "[Client] Partida creada y unido a \""
                                  << joinOk.getGameName() << "\" (id="
                                  << joinOk.getGameId() << ")." << std::endl;
                        inGame = true;
                    }
                    else if (joinResponse->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_ERROR))
                    {
                        const auto& err = static_cast<const ErrorMessage&>(*joinResponse);
                        std::cerr << "[Client] Error al unirse: " << err.getReason() << std::endl;
                    }
                }
                else if (response->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_ERROR))
                {
                    const auto& err = static_cast<const ErrorMessage&>(*response);
                    std::cerr << "[Client] Error al crear: " << err.getReason() << std::endl;
                }
            }
            else if (option == "3")
            {
                protocol.send(ListGamesMessage());

                auto listResponse = protocol.receive();
                if (listResponse->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_GAME_LIST))
                {
                    const auto& listMsg = static_cast<const GameListMessage&>(*listResponse);
                    if (listMsg.getGames().empty())
                    {
                        std::cout << "  No hay partidas disponibles." << std::endl;
                        continue;
                    }
                    std::cout << "  Partidas disponibles:" << std::endl;
                    for (const auto& game : listMsg.getGames())
                    {
                        std::cout << "    id=" << game.gameId
                                  << " nombre=" << game.gameName
                                  << " jugadores=" << static_cast<int>(game.playerCount)
                                  << "/" << static_cast<int>(game.maxPlayers)
                                  << std::endl;
                    }
                }

                std::cout << "ID de partida: ";
                std::string idStr;
                std::getline(std::cin, idStr);
                uint32_t gameId = static_cast<uint32_t>(std::stoul(idStr));

                protocol.send(JoinGameMessage(gameId));

                auto response = protocol.receive();
                if (response->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_JOIN_OK))
                {
                    const auto& joinOk = static_cast<const JoinOkMessage&>(*response);
                    std::cout << "[Client] Unido a \"" << joinOk.getGameName()
                              << "\" (id=" << joinOk.getGameId() << ")." << std::endl;
                    inGame = true;
                }
                else if (response->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_ERROR))
                {
                    const auto& err = static_cast<const ErrorMessage&>(*response);
                    std::cerr << "[Client] Error: " << err.getReason() << std::endl;
                }
            }
            else
            {
                std::cout << "Opcion invalida." << std::endl;
            }
        }

        // ---------------- 4. Game SDL con threads de red ----------------
        if (inGame)
        {
            PlayerDto playerDto;
            playerDto.nombre   = username;
            playerDto.xpos     = 1500;
            playerDto.ypos     = 1200;
            playerDto.hp       = 100;
            playerDto.hpMax    = 100;
            playerDto.mana     = 100;
            playerDto.manaMax  = 100;
            playerDto.level    = 1;
            playerDto.oro      = 2000;

            GameClient gameClient(socket, 1, playerDto);
            gameClient.run();
        }
    }
    catch (const ClosedSocket&)
    {
        try {
            MainMenuScreen errMenu(renderer, windowW, windowH, FONT_PATH);
            errMenu.setError("Conexion cerrada por el servidor.");
            errMenu.run();
        } catch (...) {}
        return 1;
    }
    catch (const std::exception& e)
    {
        try {
            MainMenuScreen errMenu(renderer, windowW, windowH, FONT_PATH);
            errMenu.setError(std::string("Error: ") + e.what());
            errMenu.run();
        } catch (...) {}
        return 1;
    }

    return 0;
}

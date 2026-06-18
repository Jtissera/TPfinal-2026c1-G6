#include "GameClient.h"

GameClient::GameClient(Socket &socket, uint32_t idPlayer,
                       const PlayerDto &playerDto, SDL_Window *window,
                       SDL_Renderer *renderer, const std::string &mapPath)
    : socket(socket), idPlayer(idPlayer), playerDto(playerDto), window(window),
      renderer(renderer), factory(),
      senderProtocol(factory.createProtocol(socket)),
      receiverProtocol(factory.createProtocol(socket)),
      sender(senderProtocol, sendQueue),
      receiver(receiverProtocol, receiveQueue), gameLoop(),
      mapPath(mapPath)
{
}

void GameClient::run()
{
  if (window == nullptr || renderer == nullptr)
  {
    std::cerr << "[GameClient] window/renderer inválidos" << std::endl;
    return;
  }
  // Arrancamos los hilos de red.
  sender.start();
  receiver.start();

  constexpr int GAME_W = 1280;
  constexpr int GAME_H = 720;
  const bool useFullscreen = false; // luego viene de config

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

  if (useFullscreen)
  {
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
  }
  else
  {
    SDL_SetWindowFullscreen(window, 0);
    SDL_SetWindowSize(window, GAME_W, GAME_H);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED,
                          SDL_WINDOWPOS_CENTERED);
  }

  SDL_RenderSetLogicalSize(renderer, GAME_W, GAME_H);

  // Limpiamos cualquier frame anterior del lobby.
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);

  const int FPS = 30;
  const int frameDelay = 1000 / FPS;

  gameLoop.init(window, renderer, sendQueue, receiveQueue, playerDto, mapPath);

  while (gameLoop.running())
  {
    const Uint32 frameStart = SDL_GetTicks();

    gameLoop.handleEvents();
    gameLoop.update();
    gameLoop.render();

    const int frameTime = static_cast<int>(SDL_GetTicks() - frameStart);

    if (frameDelay > frameTime)
    {
      SDL_Delay(frameDelay - frameTime);
    }
  }

  const bool disconnectedByServer = receiver.wasClosedByError();
  gameLoop.clean();

  socket.shutdown(SHUT_RDWR);

  sendQueue.close();
  receiveQueue.close();

  sender.stop();
  receiver.stop();

  sender.join();
  receiver.join();

  if (disconnectedByServer)
  {
    connectionLost = true;
    SDL_Event quitEvent;
    quitEvent.type = SDL_QUIT;
    SDL_PushEvent(&quitEvent);
  }

  SDL_RenderSetLogicalSize(renderer, 0, 0);
}

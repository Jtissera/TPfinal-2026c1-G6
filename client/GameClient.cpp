#include "GameClient.h"

GameClient::GameClient(Socket& socket, uint32_t idPlayer, const PlayerDto& playerDto)
    : idPlayer(idPlayer),
      playerDto(playerDto),
      factory(),
      protocol(factory.createProtocol(socket)),
      sender(protocol, sendQueue),
      receiver(protocol, receiveQueue),
      gameLoop() {}

void GameClient::run() {
    sender.start();
    receiver.start();

    const int FPS = 30;
    const int frameDelay = 1000 / FPS;

    this->gameLoop.init("Argentum", 1280, 720, false, sendQueue, receiveQueue, playerDto);

    while (gameLoop.running()) {
        Uint32 frameStart = SDL_GetTicks();

        gameLoop.handleEvents();
        gameLoop.update();
        gameLoop.render();

        int frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime)
            SDL_Delay(frameDelay - frameTime);
    }

    gameLoop.clean();
    sendQueue.close();
    receiveQueue.close();
    sender.stop();
    receiver.stop();
    sender.join();
    receiver.join();
}

#include "GameClient.h"

GameClient::GameClient(Socket& socket, uint32_t idPlayer, const PlayerDto& playerDto)
    : idPlayer(idPlayer),
      playerDto(playerDto),
      factory(),
      senderProtocol(factory.createProtocol(socket)),
      receiverProtocol(factory.createProtocol(socket)),
      sender(senderProtocol, sendQueue),
      receiver(receiverProtocol, receiveQueue),
      gameLoop() {}

void GameClient::run() {
       sender.start();
       receiver.start();
       const int FPS = 30;
       const int frameDelay = 1000/FPS;
       Uint32 frameStart;
       int frameTime;

       this->gameLoop.init("Argentum",1080,640,false,sendQueue,receiveQueue,playerDto);
       while (gameLoop.running()) {
              frameStart = SDL_GetTicks();
              gameLoop.handleEvents();

              gameLoop.update();
              gameLoop.render();
              frameTime = SDL_GetTicks() - frameStart;
              if (frameDelay > frameTime) {
                     SDL_Delay(frameDelay - frameTime);
              }
       }
       gameLoop.clean();
       sendQueue.close();
       receiveQueue.close();
       sender.stop();
       receiver.stop();
       sender.join();
       receiver.join();
}

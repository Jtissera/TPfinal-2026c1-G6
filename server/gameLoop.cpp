#include "gameLoop.h"

#include "../common/network/messages/server/auth/connectOKMessage.h"

GameLoop::GameLoop(Queue<ClientMessage> &gameQueue, Monitor &monitor) : gameQueue(gameQueue), monitor(monitor) {}

void GameLoop::run() {
    try {
        while (true) {
            ClientMessage incoming = gameQueue.pop();

            if (incoming.message->opCode() ==
                static_cast<uint8_t>(ClientOpCode::MSG_MOVE)) {

                const auto& move =
                    static_cast<const MoveMessage&>(*incoming.message);

                switch (move.getDirection()) {
                    case Direction::UP:    playerY -= SPEED; break;
                    case Direction::DOWN:  playerY += SPEED; break;
                    case Direction::LEFT:  playerX -= SPEED; break;
                    case Direction::RIGHT: playerX += SPEED; break;
                    default: break;
                }

                auto response = std::make_shared<const EntityMoveMessage>(
                    static_cast<uint8_t>(incoming.clientId), playerX, playerY);
                monitor.sendTo(incoming.clientId, response);
                }
        }
    }
    catch (const ClosedQueue&) {}
    catch (const std::exception& e) {
        std::cerr << "[GameLoop] error: " << e.what() << std::endl;
    }
}


void GameLoop::stop()
{
    Thread::stop();
    gameQueue.close();
}
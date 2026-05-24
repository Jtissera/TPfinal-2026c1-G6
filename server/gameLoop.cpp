#include "gameLoop.h"
#include "../common/network/messages/server/player/EntityMoveMessage.h"
#include "../common/network/messages/client/movement/moveMessage.h"
#include "../common/network/protocol/clientOpCode.h"

GameLoop::GameLoop(Queue<ClientMessage>& q, Monitor& m, GameWorld& w)
    : gameQueue(q), monitor(m), world(w) {}


//GameLoop::GameLoop(Queue<ClientMessage> &gameQueue, Monitor &monitor) : gameQueue(gameQueue), monitor(monitor) {}

void GameLoop::run() {
    try {
        while (true) {
            ClientMessage incoming = gameQueue.pop();

            if (incoming.message->opCode() ==
                static_cast<uint8_t>(ClientOpCode::MSG_MOVE)) {

                const auto& move =
                    static_cast<const MoveMessage&>(*incoming.message);
                uint32_t id = incoming.clientId;

                if (world.movePlayer(id, move.getDirection())) {
                    auto response = std::make_shared<const EntityMoveMessage>(
                        static_cast<uint8_t>(id),
                        world.getX(id),
                        world.getY(id));
                    monitor.sendTo(id, response);
                }
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
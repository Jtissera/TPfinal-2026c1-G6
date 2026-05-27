#include "gameLoop.h"


GameLoop::GameLoop(Queue<ClientMessage>& q, Monitor& m, GameWorld& w)
    : gameQueue(q), monitor(m), world(w) {}


void GameLoop::run() {
    int eventCounter = 0;
    const int TICKS_PER_UPDATE = 100;
    
    try {
        while (true) {
            ClientMessage incoming;

            while (gameQueue.try_pop(incoming)) {
                processMessage(incoming);
                eventCounter++;

                if (eventCounter >= TICKS_PER_UPDATE) {
                    worldUpdate();
                    eventCounter = 0;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    } catch (const ClosedQueue&) {
    } catch (const std::exception& e) {
        std::cerr << "[GameLoop] error: " << e.what() << std::endl;
    }
}

void GameLoop::processMessage(const ClientMessage& incoming) {
    uint8_t opcode = incoming.message->opCode();
    uint32_t id = incoming.clientId;

    if (opcode == static_cast<uint8_t>(ClientOpCode::MSG_MOVE)) {
        const auto& move = static_cast<const MoveMessage&>(*incoming.message);
        if (world.movePlayer(id, move.getDirection())) {
            auto response = std::make_shared<const EntityMoveMessage>(
                (uint8_t)id, world.getX(id), world.getY(id));
            monitor.sendTo(id, response);
        }
    }
}

void GameLoop::worldUpdate() {

    auto changed = world.tick(0.05f); 
    for (uint32_t id : changed) {
        statManager.sendPlayerStats(id, world, monitor);
    }
}

void GameLoop::stop() {
    Thread::stop();
    gameQueue.close(); 
}
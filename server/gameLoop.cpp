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
    dispatcher.dispatch(incoming, world, monitor);  
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

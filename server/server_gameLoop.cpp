#include "server_gameLoop.h"

GameLoop::GameLoop(Queue<Command>& q, Monitor& m)
    : gameQueue(q), monitor(m) {}

void GameLoop::run() {
    try {
        while (true) {
            Command cmd = gameQueue.pop();

            std::cout << "[GameLoop] Comando de " << cmd.playerName << ": type=" << (int)cmd.type << std::endl;

            // echo al cliente como mensaje
            Message msg;
            msg.type = cmd.type;
            msg.text = "echo";
            monitor.broadcast(msg);
        }
    } catch (const ClosedQueue&) {
    } catch (const std::exception& e) {
        std::cerr << "[GameLoop] Error: " << e.what() << std::endl;
    }
}

void GameLoop::stop() {
    Thread::stop();
    gameQueue.close();
}
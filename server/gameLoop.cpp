#include "gameLoop.h"

#include "../common/network/messages/server/auth/connectOKMessage.h"

GameLoop::GameLoop(Queue<ClientMessage> &gameQueue, Monitor &monitor) : gameQueue(gameQueue), monitor(monitor) {}

void GameLoop::run()
{
    try
    {
        while (true)
        {
            ClientMessage incoming = gameQueue.pop();

            std::cout << "[GameLoop] client=" << incoming.clientId
                      << " opcode=0x" << std::hex
                      << static_cast<int>(incoming.message->opCode())
                      << std::dec << std::endl;

            // Aca hay que manejar la llegada de mensajes
            auto response = std::make_shared<const ConnectOkMessage>();
            monitor.sendTo(incoming.clientId, response);
        }
    }
    catch (const ClosedQueue &)
    {
    }
    catch (const std::exception &e)
    {
        std::cerr << "[GameLoop] error: " << e.what() << std::endl;
    }
}

void GameLoop::stop()
{
    Thread::stop();
    gameQueue.close();
}
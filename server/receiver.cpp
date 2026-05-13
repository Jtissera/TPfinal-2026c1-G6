#include "receiver.h"

Receiver::Receiver(Protocol protocol, uint32_t clientId, Queue<ClientMessage> &gameQueue)
    : protocol(std::move(protocol)),
      clientId(clientId),
      gameQueue(gameQueue) {}

void Receiver::run()
{
    try
    {
        while (true)
        {
            auto message = protocol.receive();
            gameQueue.push(ClientMessage{clientId, std::move(message)});
        }
    }
    catch (const ClosedSocket &)
    {
    }
    catch (const LibError &)
    {
    }
    catch (const std::exception &e)
    {
        std::cerr << "[Receiver] client=" << clientId << " error: " << e.what() << std::endl;
    }
}

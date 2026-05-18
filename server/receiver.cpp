#include "receiver.h"

Receiver::Receiver(Protocol protocol,
                   uint32_t clientId,
                   Queue<ClientMessage> &lobbyQueue)
    : protocol(std::move(protocol)),
      clientId(clientId),
      currentQueue(&lobbyQueue) {}

void Receiver::setQueue(Queue<ClientMessage> &newQueue)
{
    currentQueue.store(&newQueue);
}

void Receiver::run()
{
    try
    {
        while (true)
        {
            auto message = protocol.receive();
            currentQueue.load()->push(ClientMessage{clientId, std::move(message)});
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
        std::cerr << "[Receiver] client=" << clientId
                  << " error: " << e.what() << std::endl;
    }
}
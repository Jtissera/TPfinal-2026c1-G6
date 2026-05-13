#include "sender.h"

Sender::Sender(Protocol protocol, Queue<std::shared_ptr<const Message>> &clientQueue)
    : protocol(std::move(protocol)),
      clientQueue(clientQueue) {}

void Sender::run()
{
    try
    {
        while (true)
        {
            auto message = clientQueue.pop();
            protocol.send(*message);
        }
    }
    catch (const ClosedQueue &)
    {
    }
    catch (const LibError &)
    {
    }
    catch (const std::exception &e)
    {
        std::cerr << "[Sender] error: " << e.what() << std::endl;
    }
}

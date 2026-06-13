

#include "ClientReceiver.h"
#include <iostream>

ClientReceiver::ClientReceiver(
    Protocol protocol, Queue<std::shared_ptr<const Message>> &clientQueue)
    : protocol(std::move(protocol)), clientQueue(clientQueue) {}

void ClientReceiver::run()
{
  try
  {
    while (true)
    {
      auto msg = protocol.receive();
      clientQueue.push(std::shared_ptr<const Message>(std::move(msg)));
    }
  }
  catch (const ClosedSocket &)
  {
    closedByError = true;
  }
  catch (const ClosedQueue &)
  {
    std::cerr << "[ClientReceiver] Cola cerrada, terminando receptor." << std::endl;
  }
  catch (const std::exception &e)
  {
    std::cerr << "[ClientReceiver] Error inesperado: " << e.what() << std::endl;
  }

  clientQueue.close();
}

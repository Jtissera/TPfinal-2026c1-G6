#include "client_client.h"

#include "../common/network/messages/client/auth/connectMessage.h"
#include "../common/network/protocol/clientOpCode.h"

static constexpr uint8_t PROTOCOL_VERSION = 0x01; // A modificar

Client::Client(const char *hostname, const char *servname)
    : socket(hostname, servname),
      factory(),
      protocol(factory.createProtocol(socket)) {}

int Client::run()
{
    try
    {
        std::cout << "Nombre de jugador: ";
        std::string username;
        std::getline(std::cin, username);

        protocol.send(ConnectMessage(PROTOCOL_VERSION, username));

        auto response = protocol.receive();

        std::cout << "[Client] response opcode=0x"
                  << std::hex << static_cast<int>(response->opCode())
                  << std::dec << std::endl;
    }
    catch (const ClosedSocket &)
    {
        std::cerr << "[Client] Connection closed by server." << std::endl;
        return 1;
    }
    catch (const LibError &e)
    {
        std::cerr << "[Client] Connection error: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[Client] Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
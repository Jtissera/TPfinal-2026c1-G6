#include "charHandler.h"

CharHandler::CharHandler(Queue<ClientMessage>& charQueue,
                         Monitor& charMonitor,
                         PlayerRepository& playerRepo,
                         PlayerFactory& playerFactory,
                         ReceiverRegistry& receiverRegistry,
                         Queue<ClientMessage>& lobbyQueue,
                         ClientRegistry& clientRegistry)
    : charQueue(charQueue),
      charMonitor(charMonitor),
      playerRepo(playerRepo),
      playerFactory(playerFactory),
      receiverRegistry(receiverRegistry),
      lobbyQueue(lobbyQueue),
      clientRegistry(clientRegistry) {}

void CharHandler::run() {
    try {
        while (true) {
            ClientMessage incoming = charQueue.pop();
            uint8_t opcode = incoming.message->opCode();

            if (opcode == static_cast<uint8_t>(ClientOpCode::MSG_CREATE_CHAR)) {
                handleCreateChar(incoming.clientId, *incoming.message);
            } else {
                auto error = std::make_shared<const ErrorMessage>("Expected create char");
                charMonitor.sendTo(incoming.clientId, error);
            }
        }
    }
    catch (const ClosedQueue&) {}
    catch (const std::exception& e) {
        std::cerr << "[CharHandler] error: " << e.what() << std::endl;
    }
}

void CharHandler::stop() {
    Thread::stop();
    charQueue.close();
}

void CharHandler::handleCreateChar(uint32_t clientId, const Message& message) {
    const auto& msg = static_cast<const CreateCharMessage&>(message);

    try {
        Player player = playerFactory.create(
            clientId,
            msg.getName(),
            msg.getRaza(),
            msg.getClase(),
            6 * 96, 7 * 96 
        );
        playerRepo.save(clientId, std::move(player));

        // Swap a lobbyQueue como hace lobby
        auto* receiver = receiverRegistry.get(clientId);
        if (receiver)
            receiver->setQueue(lobbyQueue);

        charMonitor.removeQueue(clientId);

        auto* clientQueue = clientRegistry.get(clientId);
        if (clientQueue)
            clientQueue->try_push(std::make_shared<const CreateOkMessage>());

    }
    catch (const std::exception& e) {
        auto error = std::make_shared<const ErrorMessage>(e.what());
        charMonitor.sendTo(clientId, error);
    }
}
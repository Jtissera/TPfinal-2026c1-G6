#include "moveHandler.h"

void MoveHandler::handle(uint32_t clientId,
                         const Message &msg,
                         GameWorld &world,
                         Monitor &monitor)
{
    Player &player = world.getPlayer(clientId);
    if (player.isMeditating())
    {
        return;
    }

    const MoveMessage &moveMsg = static_cast<const MoveMessage &>(msg);
    const Direction direction = moveMsg.getDirection();
    const bool moving = moveMsg.isMoving();

    if (!moving)
    {
        monitor.broadcast(std::make_shared<const EntityMoveMessage>(
            clientId,
            world.getPixelX(clientId),
            world.getPixelY(clientId),
            direction,
            false));
        return;
    }

    if (world.movePlayer(clientId, direction))
    {
        monitor.broadcast(std::make_shared<const EntityMoveMessage>(
            clientId,
            world.getPixelX(clientId),
            world.getPixelY(clientId),
            direction,
            true));
    }
}
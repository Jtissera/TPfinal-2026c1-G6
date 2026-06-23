#include "clanHandler.h"

void ClanHandler::handle(uint32_t clientId,
                         const Message &msg,
                         GameWorld &world,
                         Monitor &monitor)
{
    if (!world.hasPlayer(clientId))
    {
        return;
    }

    const ClanSyncMessage &syncMsg =
        static_cast<const ClanSyncMessage &>(msg);

    Player &player = world.getPlayer(clientId);
    player.setClanName(syncMsg.getClanName());
    player.setClanFounder(syncMsg.getIsFounder());

    monitor.broadcast(std::make_shared<const ClanUpdateMessage>(
        clientId,
        syncMsg.getClanName(),
        syncMsg.getIsFounder()));
}
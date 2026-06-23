#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"
#include "server/game/items/item.h"
#include <cstdint>

class ItemOnGroundMessage : public Message
{
public:
    ItemOnGroundMessage(Item item, int x, int y);

    const Item &getItem() const;
    int getX() const;
    int getY() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    Item item;
    int x;
    int y;
};
#ifndef MAP_CHANGED_MESSAGE_H
#define MAP_CHANGED_MESSAGE_H

#include <string>
#include "../../message.h"
#include "../../../protocol/serverOpCode.h"

class MapChangedMessage : public Message
{
private:
    std::string mapPath;

public:
    explicit MapChangedMessage(std::string mapPath);

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

    const std::string &getMapPath() const;
};

#endif // MAP_CHANGED_MESSAGE_H
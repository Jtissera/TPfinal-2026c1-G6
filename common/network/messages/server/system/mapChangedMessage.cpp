#include "mapChangedMessage.h"

MapChangedMessage::MapChangedMessage(std::string mapPath)
    : mapPath(std::move(mapPath))
{
}

const std::string &MapChangedMessage::getMapPath() const
{
    return mapPath;
}

uint8_t MapChangedMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_MAP_CHANGED);
}

void MapChangedMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeString(mapPath);
}
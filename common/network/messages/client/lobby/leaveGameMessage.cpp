#include "leaveGameMessage.h"

uint8_t LeaveGameMessage::opCode() const
{
  return static_cast<uint8_t>(ClientOpCode::MSG_LEAVE_GAME);
}

void LeaveGameMessage::serializeBody(PacketWriter &writer) const
{
  (void)writer;
}
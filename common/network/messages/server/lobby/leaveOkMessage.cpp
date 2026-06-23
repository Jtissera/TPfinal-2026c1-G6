#include "leaveOkMessage.h"

uint8_t LeaveOkMessage::opCode() const
{
  return static_cast<uint8_t>(ServerOpCode::MSG_LEAVE_OK);
}

void LeaveOkMessage::serializeBody(PacketWriter &writer) const
{
  (void)writer;
}
#pragma once
#include "cheatType.h"
#include "common/network/messages/message.h"
#include "common/network/protocol/clientOpCode.h"
#include "common/network/protocol/packetWriter.h"
#include <cstdint>

class CheatMessage : public Message
{
public:
  explicit CheatMessage(CheatType cheat);

  CheatType getCheat() const;

  uint8_t opCode() const override;
  void serializeBody(PacketWriter &writer) const override;

private:
  CheatType cheat;
};
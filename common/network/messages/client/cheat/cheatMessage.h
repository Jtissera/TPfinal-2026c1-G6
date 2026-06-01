#pragma once

#include "cheatType.h"
#include "common/network/messages/message.h"
#include "common/network/protocol/clientOpCode.h"

class CheatMessage : public Message {
public:
  explicit CheatMessage(CheatType cheat) : cheat(cheat) {}

  uint8_t opCode() const override {
    return static_cast<uint8_t>(ClientOpCode::MSG_CHEAT);
  }

  void serializeBody(PacketWriter &writer) const override {
    writer.writeUint8(static_cast<uint8_t>(cheat));
  }

  CheatType getCheat() const { return cheat; }

private:
  CheatType cheat;
};
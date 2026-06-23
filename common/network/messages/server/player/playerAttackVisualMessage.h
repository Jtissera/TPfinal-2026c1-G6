#ifndef PLAYER_ATTACK_VISUAL_MESSAGE_H
#define PLAYER_ATTACK_VISUAL_MESSAGE_H

#include <cstdint>

#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"

enum class PlayerAttackVisualType : uint8_t {
    Physical = 0,
    Magic = 1,
    Ranged = 2,
    Heal = 3
};

class PlayerAttackVisualMessage : public Message {
private:
    uint32_t attackerId;
    uint32_t targetId;
    PlayerAttackVisualType visualType;
    std::string effectId;

public:
    PlayerAttackVisualMessage(uint32_t attackerId,
                              uint32_t targetId,
                              PlayerAttackVisualType visualType,
                              std::string effectId);

    uint8_t opCode() const override;
    void serializeBody(PacketWriter& writer) const override;

    uint32_t getAttackerId() const;
    uint32_t getTargetId() const;
    PlayerAttackVisualType getVisualType() const;
    const std::string &getEffectId() const;
};

#endif

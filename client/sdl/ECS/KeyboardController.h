#pragma once
#include "ECS.h"
#include "TransformComponent.h"
#include "SpriteComponent.h"
#include "common/queue.h"
#include <SDL2/SDL.h>

enum class FacingDirection { Down, Up, Left, Right };

class KeyboardController : public Component {
public:
    explicit KeyboardController(Queue<std::shared_ptr<const Message>>& sendQueue);

    void init()                     override;
    void update(UpdateContext& ctx) override;

    FacingDirection getLastDirection() const { return lastDirection; }
    bool            isHoldingKey()     const { return holdingKey;    }

private:
    TransformComponent* transform     = nullptr;
    SpriteComponent*    sprite        = nullptr;
    FacingDirection     lastDirection = FacingDirection::Down;
    bool                holdingKey    = false;

    Queue<std::shared_ptr<const Message>>& sendQueue;
    Uint32 lastSendMs = 0;
    static constexpr Uint32 SEND_INTERVAL_MS = 250;
};
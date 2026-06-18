
#include "pickUpSystem.h"

#include "ECS/Components.h"
#include "common/network/messages/client/inventory/pickItemMessage.h"
#include <iostream>

bool PickupSystem::handleMouseClick(
    int screenX,
    int screenY,
    const std::vector<GroundPickupTarget>& targets,
    Queue<std::shared_ptr<const Message>>* sendQueue
) {
    for (const GroundPickupTarget& target : targets) {
        if (target.entity == nullptr) continue;

        if (!target.entity->hasComponent<SpriteComponent>()) continue;

        const auto& sprite = target.entity->getComponent<SpriteComponent>();
        SDL_Rect clickableRect = sprite.getDestRect();

        // Margen extra para que sea mas facil clickear un sprite chico.
        clickableRect.x -= 10;
        clickableRect.y -= 10;
        clickableRect.w += 20;
        clickableRect.h += 20;

        const bool clickedTarget =
            screenX >= clickableRect.x &&
            screenX <= clickableRect.x + clickableRect.w &&
            screenY >= clickableRect.y &&
            screenY <= clickableRect.y + clickableRect.h;

        if (!clickedTarget) continue;

        sendPickMessage(target.instanceId, target.isGold, sendQueue);
        return true;
    }

    return false;
}

void PickupSystem::sendPickMessage(
    uint32_t instanceId,
    bool isGold,
    Queue<std::shared_ptr<const Message>>* sendQueue
) {
    if (sendQueue == nullptr) return;

    sendQueue->try_push(std::make_shared<const PickItemMessage>(instanceId, isGold));

    std::cout << "[PICKUP] PickItemMessage enviado instanceId=" << instanceId
              << " isGold=" << isGold << std::endl;
}

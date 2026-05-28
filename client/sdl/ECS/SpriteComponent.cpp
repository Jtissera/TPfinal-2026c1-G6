#include "SpriteComponent.h"

#include <utility>

#include "../AssetManager.h"
#include "../TextureManager.h"

SpriteComponent::SpriteComponent(
    AssetManager& assets,
    const std::string& id,
    bool isAnimated,
    std::map<std::string, Animation> anims,
    SpriteSheetConfig config
)
    : assets(assets),
      animations(std::move(anims)),
      animated(isAnimated),
      frameWidth(config.frameWidth),
      frameHeight(config.frameHeight),
      scale(config.scale),
      startX(config.startX),
      startY(config.startY) {

    if (animations.count("IdleDown") > 0) {
        Play("IdleDown");
    } else if (animations.count("Idle") > 0) {
        Play("Idle");
    } else if (!animations.empty()) {
        Play(animations.begin()->first.c_str());
    }

    setText(id);
}

void SpriteComponent::setText(const std::string& id) {
    bodyTexture = assets.GetTexture(id);
}

void SpriteComponent::setHeadTexture(const std::string& textureId, int selectedHeadIndex) {
    headTexture = assets.GetTexture(textureId);
    headIndex = selectedHeadIndex;
    hasHead = headTexture != nullptr;
}

void SpriteComponent::Play(const char* animName) {
    std::string name(animName);

    if (animations.count(name) == 0) {
        return;
    }

    if (currentAnim == name) {
        return;
    }

    currentAnim = name;
    frames = animations[name].frames;
    animationIndex = animations[name].index;
    speed = animations[name].speed;
}

void SpriteComponent::init() {
    transform = &entity->getComponent<TransformComponent>();

    srcRect.x = startX;
    srcRect.y = startY + animationIndex * frameHeight;
    srcRect.w = frameWidth;
    srcRect.h = frameHeight;
}

void SpriteComponent::update(UpdateContext& context) {
    if (animated && frames > 0) {
        int currentFrame = static_cast<int>((SDL_GetTicks() / speed) % frames);
        srcRect.x = startX + currentFrame * frameWidth;
    } else {
        srcRect.x = startX;
    }

    srcRect.y = startY + animationIndex * frameHeight;
    // Coordenadas de mundo -> pantalla.
    destRect.x = static_cast<int>(transform->position.x) - context.camera.x;
    destRect.y = static_cast<int>(transform->position.y) - context.camera.y + 133;

    // Tamaño visual.
    destRect.w = frameWidth * scale;
    destRect.h = frameHeight * scale;
}

void SpriteComponent::draw(RenderContext& context) {

    if (bodyTexture == nullptr) {
        return;
    }

    context.textureManager.Draw(bodyTexture, srcRect, destRect, spriteFlip);

    if (hasHead && headTexture != nullptr) {
        SDL_Rect headSrc{};
        SDL_Rect headDst{};

        headSrc.x = headStartX + headIndex * headStepX;

        int headDirectionRow = 0;

        if (animationIndex == 0) {
            headDirectionRow = 0;
        } else if (animationIndex == 1) {
            headDirectionRow = 1;
        } else if (animationIndex == 2) {
            headDirectionRow = 2;
        } else if (animationIndex == 3) {
            headDirectionRow = 3;
        }

        headSrc.y = headStartY + headDirectionRow * headStepY;
        headSrc.w = headFrameWidth;
        headSrc.h = headFrameHeight;

        headDst.w = 23;
        headDst.h = 23;

        if (animationIndex == 0) {
            headDst.x = destRect.x + 18;
            headDst.y = destRect.y - 3;
        } else if (animationIndex == 1) {
            headDst.x = destRect.x + 18;
            headDst.y = destRect.y - 3;
        } else if (animationIndex == 2) {
            headDst.x = destRect.x + 18;
            headDst.y = destRect.y - 3;
        } else if (animationIndex == 3) {
            headDst.x = destRect.x + 18;
            headDst.y = destRect.y - 3;
        }

        context.textureManager.Draw(headTexture, headSrc, headDst, spriteFlip);
    }
}


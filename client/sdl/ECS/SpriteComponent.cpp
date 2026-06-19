#include "SpriteComponent.h"

#include "../AssetManager.h"
#include "../TextureManager.h"
#include <iostream>
#include <utility>

SpriteComponent::SpriteComponent(AssetManager &assets, const std::string &id,
                                 bool isAnimated,
                                 std::map<std::string, Animation> anims,
                                 SpriteSheetConfig config)
    : assets(assets), animations(std::move(anims)), animated(isAnimated),
      frameWidth(config.frameWidth), frameHeight(config.frameHeight),
      scale(config.scale), startX(config.startX), startY(config.startY),
      renderOffsetX(config.renderOffsetX), renderOffsetY(config.renderOffsetY)
{

    if (animations.count("IdleDown") > 0)
    {
        Play("IdleDown");
    }
    else if (animations.count("Idle") > 0)
    {
        Play("Idle");
    }
    else if (!animations.empty())
    {
        Play(animations.begin()->first.c_str());
    }

    setText(id);
}

void SpriteComponent::setText(const std::string &id)
{
    bodyTexture = assets.GetTexture(id);
}

void SpriteComponent::setHeadTexture(const std::string &textureId,
                                     int selectedHeadIndex)
{
    headTexture = assets.GetTexture(textureId);
    headIndex = selectedHeadIndex;
    hasHead = headTexture != nullptr;
}

void SpriteComponent::Play(const char *animName)
{
    std::string name(animName);

    if (animations.count(name) == 0)
    {
        return;
    }

    if (currentAnim == name)
    {
        return;
    }

    currentAnim = name;
    frames = animations[name].frames;
    animationIndex = animations[name].index;
    speed = animations[name].speed;
    manualFrameIndex = 0;
}

void SpriteComponent::init()
{
    transform = &entity->getComponent<TransformComponent>();

    srcRect.x = startX;
    srcRect.y = startY + animationIndex * frameHeight;
    srcRect.w = frameWidth;
    srcRect.h = frameHeight;
}

void SpriteComponent::update(UpdateContext &context)
{
    if (animated && frames > 0)
    {
        if (isManualAnimation)
        {
            srcRect.x = startX + (manualFrameIndex * frameWidth);
        }
        else
        {
            int currentFrame = static_cast<int>((SDL_GetTicks() / speed) % frames);
            srcRect.x = startX + currentFrame * frameWidth;
        }
    }
    else
    {
        srcRect.x = startX;
    }

    srcRect.y = startY + animationIndex * frameHeight;
    srcRect.w = frameWidth;
    srcRect.h = frameHeight;

    destRect.x = static_cast<int>(transform->position.x) - context.camera.x -
                 (frameWidth * scale / 2);
    destRect.y = static_cast<int>(transform->position.y) - context.camera.y +
                 133 - (frameHeight * scale);
    destRect.w = frameWidth * scale;
    destRect.h = frameHeight * scale;
}

void SpriteComponent::draw(RenderContext &context)
{

    if (bodyTexture == nullptr)
    {
        return;
    }

    SDL_Rect bodyDest = destRect;

    bodyDest.x += renderOffsetX * scale;
    bodyDest.y += renderOffsetY * scale;

    context.textureManager.Draw(bodyTexture, srcRect, bodyDest, spriteFlip);

    if (hasHead && headTexture != nullptr)
    {
        SDL_Rect headSrc{};
        SDL_Rect headDst{};

        headSrc.x = headStartX + headIndex * headStepX;

        int headDirectionRow = 0;

        if (animationIndex == 0)
        {
            headDirectionRow = 0;
        }
        else if (animationIndex == 1)
        {
            headDirectionRow = 1;
        }
        else if (animationIndex == 2)
        {
            headDirectionRow = 2;
        }
        else if (animationIndex == 3)
        {
            headDirectionRow = 3;
        }

        headSrc.y = headStartY + headDirectionRow * headStepY;
        headSrc.w = headFrameWidth;
        headSrc.h = headFrameHeight;

        headDst.w = 23;
        headDst.h = 23;

        if (animationIndex == 0)
        {
            headDst.x = destRect.x + 18;
            headDst.y = destRect.y - 3;
        }
        else if (animationIndex == 1)
        {
            headDst.x = destRect.x + 18;
            headDst.y = destRect.y - 3;
        }
        else if (animationIndex == 2)
        {
            headDst.x = destRect.x + 18;
            headDst.y = destRect.y - 3;
        }
        else if (animationIndex == 3)
        {
            headDst.x = destRect.x + 18;
            headDst.y = destRect.y - 3;
        }

        context.textureManager.Draw(headTexture, headSrc, headDst, spriteFlip);
        if (hasHelmet && helmetTexture != nullptr)
        {
            SDL_Rect helmetSrc{};

            int dirIndex = animationIndex % 4;

            if (dirIndex == 0)
            {
                helmetSrc.x = helmetDownSrcX;
                helmetSrc.y = helmetDownSrcY;
            }
            else if (dirIndex == 1)
            {
                helmetSrc.x = helmetUpSrcX;
                helmetSrc.y = helmetUpSrcY;
            }
            else if (dirIndex == 2)
            {
                helmetSrc.x = helmetLeftSrcX;
                helmetSrc.y = helmetLeftSrcY;
            }
            else
            {
                helmetSrc.x = helmetRightSrcX;
                helmetSrc.y = helmetRightSrcY;
            }

            helmetSrc.w = helmetSrcW;
            helmetSrc.h = helmetSrcH;

            SDL_Rect helmetDst{};

            helmetDst.w = helmetSrcW * 8 / 5;
            helmetDst.h = helmetSrcH * 8 / 5;

            helmetDst.x = headDst.x + (headDst.w / 2) - (helmetDst.w / 2) + helmetOffsetX;
            helmetDst.y = headDst.y + (headDst.h / 2) - (helmetDst.h / 2) + helmetOffsetY;

            SDL_RendererFlip helmetFlip = SDL_FLIP_NONE;

            context.textureManager.Draw(helmetTexture, helmetSrc, helmetDst, helmetFlip);

        }
    }
}
const SDL_Rect &SpriteComponent::getSrcRect() const { return srcRect; }

const SDL_Rect &SpriteComponent::getDestRect() const { return destRect; }
int SpriteComponent::getStartX() const { return startX; }

int SpriteComponent::getStartY() const { return startY; }

void SpriteComponent::setSpriteTextureAndConfig(
    const std::string &newTextureId, const SpriteSheetConfig &newConfig)
{
    // Cambia la textura principal del cuerpo/personaje.
    setText(newTextureId);

    // Actualiza la metadata del spritesheet.
    frameWidth = newConfig.frameWidth;
    frameHeight = newConfig.frameHeight;
    scale = newConfig.scale;

    // Actualiza desde dónde empieza el bloque del sprite.
    startX = newConfig.startX;
    startY = newConfig.startY;

    renderOffsetX = newConfig.renderOffsetX;
    renderOffsetY = newConfig.renderOffsetY;

    // Asegura que el rectángulo fuente tenga dimensiones válidas.
    srcRect.w = frameWidth;
    srcRect.h = frameHeight;
    std::cout << "[SPRITE CONFIG] texture=" << newTextureId << " frame=("
              << newConfig.frameWidth << "x" << newConfig.frameHeight << ")"
              << " start=(" << newConfig.startX << "," << newConfig.startY << ")"
              << " offset=(" << newConfig.renderOffsetX << ","
              << newConfig.renderOffsetY << ")" << std::endl;
}

void SpriteComponent::setRenderOffset(int offsetX, int offsetY)
{
    renderOffsetX = offsetX;
    renderOffsetY = offsetY;
}

void SpriteComponent::setHelmetTexture(const std::string &textureId,
                                       int offsetX, int offsetY, int srcW,
                                       int srcH, int downSrcX, int downSrcY,
                                       int leftSrcX, int leftSrcY,
                                       int rightSrcX, int rightSrcY, int upSrcX,
                                       int upSrcY)
{
    // Pedimos la textura al AssetManager.
    helmetTexture = assets.GetTexture(textureId);

    // Guardamos offset visual.
    helmetOffsetX = offsetX;
    helmetOffsetY = offsetY;

    // Guardamos tamaño del recorte.
    helmetSrcW = srcW;
    helmetSrcH = srcH;

    // Guardamos recortes por dirección.
    helmetDownSrcX = downSrcX;
    helmetDownSrcY = downSrcY;

    helmetLeftSrcX = leftSrcX;
    helmetLeftSrcY = leftSrcY;

    helmetRightSrcX = rightSrcX;
    helmetRightSrcY = rightSrcY;

    helmetUpSrcX = upSrcX;
    helmetUpSrcY = upSrcY;

    // Solo se dibuja si existe la textura.
    hasHelmet = helmetTexture != nullptr;
}

void SpriteComponent::clearHelmet()
{
    helmetTexture = nullptr;
    hasHelmet = false;

    helmetOffsetX = 0;
    helmetOffsetY = 0;

    helmetSrcX = 0;
    helmetSrcY = 0;
    helmetSrcW = 32;
    helmetSrcH = 32;
}

void SpriteComponent::setBody(const std::string &textureId,
                              const SpriteSheetConfig &config)
{
    setText(textureId);

    frameWidth = config.frameWidth;
    frameHeight = config.frameHeight;
    scale = config.scale;
    startX = config.startX;
    startY = config.startY;
}

void SpriteComponent::clearHead()
{
    headTexture = nullptr;
    hasHead = false;
    headIndex = 0;
}

void SpriteComponent::setManualAnimation(bool manual)
{
    isManualAnimation = manual;
}

void SpriteComponent::StepFrame()
{
    if (frames > 0)
    {
        manualFrameIndex = (manualFrameIndex + 1) % frames;
    }
}

#include "SpriteComponent.h"

#include "SpriteSheetConfig.h"
#include "../TextureManager.h"
#include "../../Game.h"

SpriteComponent::SpriteComponent(const char* path) {
    setText(path);
}

SpriteComponent::SpriteComponent(const std::string& id, bool isAnimated) {
    animated = isAnimated;
    // index = fila (0=sur, 1=oeste, 2=este, 3=norte)
    // frames = cantidad de frames en esa fila
    // speed  = ms por frame
    animations.emplace("Idle",   Animation(0, 4,  150));
    animations.emplace("Walk",   Animation(0, 6,  100));
    //animations.emplace("Attack", Animation(0, 12, 80));
    //animations.emplace("Hurt",   Animation(0, 4,  100));
    Play("Idle");
    setText(id);
}
SpriteComponent::SpriteComponent(
    const std::string& id,
    bool isAnimated,
    std::map<std::string, Animation> anims,
    SpriteSheetConfig config
) {
    animated = isAnimated;              // Define si el sprite usa animación.
    animations = anims;                 // Guarda las animaciones disponibles.
    frameWidth = config.frameWidth;     // Guarda el ancho real del frame.
    frameHeight = config.frameHeight;   // Guarda el alto real del frame.
    scale = config.scale;               // Guarda la escala visual.
    Play("Idle");                       // Arranca con animación Idle.
    setText(id);                        // Carga la textura por id.
}


SpriteComponent::SpriteComponent(const std::string& id, bool isAnimated,
                                 std::map<std::string, Animation> anims) {
    animated = isAnimated;
    animations = anims;
    Play("Idle");
    setText(id);
}

void SpriteComponent::setText(const std::string& id) {
    bodyTexture = Game::assets->GetTexture(id);
}

void SpriteComponent::Play(const char* animName) {
    std::string name(animName);
    if (animations.count(name) == 0) return;
    frames         = animations[name].frames;
    animationIndex = animations[name].index;
    speed          = animations[name].speed;
}

void SpriteComponent::init() {
    transform = &entity->getComponent<TransformComponent>();
    srcRect.x = 0;
    srcRect.y = 0;
    srcRect.w = frameWidth;
    srcRect.h = frameHeight;
}


void SpriteComponent::update() {
    if (animated && frames > 0) {
        // Avanza horizontalmente por los frames de la fila actual.
        srcRect.x = frameWidth * static_cast<int>((SDL_GetTicks() / speed) % frames);
    } else {
        // Si no está animado, usa el primer frame.
        srcRect.x = 0;
    }

    // Selecciona la fila de la animación.
    srcRect.y = animationIndex * frameHeight;

    // Posición final en pantalla, ajustada por cámara.
    destRect.x = static_cast<int>(transform->position.x) - Game::camera.x;
    destRect.y = static_cast<int>(transform->position.y) - Game::camera.y;
    destRect.w = transform->width  * transform->scale;
    destRect.h = transform->height * transform->scale;
    destRect.y = static_cast<int>(transform->position.y) - Game::camera.y + 133;

    // Tamaño visual en pantalla.
    destRect.w = frameWidth * scale;
    destRect.h = frameHeight * scale;
}

void SpriteComponent::draw() {
    TextureManager::Draw(bodyTexture, srcRect, destRect, spriteFlip);
    if (hasHead && headTexture != nullptr) {
        SDL_Rect headSrc;
        SDL_Rect headDst;

        // Elegimos la columna de la cabeza seleccionada.
        headSrc.x = headStartX + headIndex * headStepX;

        int headDirectionRow = 0;

        // Mapeo dirección cuerpo -> fila de cabeza.
        if (animationIndex == 0) {
            // Abajo / frente.
            headDirectionRow = 0;
        } else if (animationIndex == 1) {
            // Arriba / espalda.
            headDirectionRow = 1;
        } else if (animationIndex == 2) {
            // Izquierda.
            headDirectionRow = 2;
        } else if (animationIndex == 3) {
            // Derecha.
            headDirectionRow = 3;
        }

        headSrc.y = headStartY + headDirectionRow * headStepY;
        headSrc.w = headFrameWidth;
        headSrc.h = headFrameHeight;

        // Tamaño visual de la cabeza.
        headDst.w = 22;
        headDst.h = 22;

        // Offset según dirección.
        if (animationIndex == 0) {
            // Frente.
            headDst.x = destRect.x + 10;
            headDst.y = destRect.y + 1;

        } else if (animationIndex == 1) {
            // Espalda.
            headDst.x = destRect.x + 10;
            headDst.y = destRect.y + 1;

        } else if (animationIndex == 2) {
            // Izquierda.
            headDst.x = destRect.x + 9;
            headDst.y = destRect.y + 1;

        } else if (animationIndex == 3) {
            // Derecha.
            headDst.x = destRect.x + 11;
            headDst.y = destRect.y + 1;
        }

        TextureManager::Draw(headTexture, headSrc, headDst, spriteFlip);
    }
}

void SpriteComponent::setHeadTexture(const std::string& textureId, int selectedHeadIndex) {
    // Busca la textura de cabezas previamente cargada en el AssetManager.
    headTexture = Game::assets->GetTexture(textureId);

    // Guarda qué cabeza concreta queremos usar dentro del spritesheet.
    headIndex = selectedHeadIndex;

    // Activa el dibujo de cabeza solo si la textura existe.
    hasHead = headTexture != nullptr;
}
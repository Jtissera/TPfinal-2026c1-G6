
#ifndef PRUEBA_SDL_SPRITECOMPONENT_H
#define PRUEBA_SDL_SPRITECOMPONENT_H

#include <map>
#include <string>
#include "SDL2/SDL.h"
#include "ECS.h"
#include "Animation.h"
#include "SpriteSheetConfig.h"
#include "TransformComponent.h"

class SpriteComponent : public Component {
public:
    int animationIndex = 0;
    SDL_RendererFlip spriteFlip = SDL_FLIP_NONE;
    std::map<std::string, Animation> animations;

    SpriteComponent() = default;
    explicit SpriteComponent(const char* path);
    SpriteComponent(const std::string& id, bool isAnimated, std::map<std::string, Animation> anims);
    SpriteComponent(const std::string& id, bool isAnimated);

    SpriteComponent(const std::string &id, bool isAnimated, std::map<std::string, Animation> anims,
                    SpriteSheetConfig config);

    ~SpriteComponent() = default;

    void setText(const std::string& id);
    void setHeadTexture(const std::string& textureId, int selectedHeadIndex);
    void Play(const char* animName);


    void init() override;
    void update() override;
    void draw() override;

private:
    TransformComponent* transform = nullptr;

    SDL_Texture* bodyTexture = nullptr;
    SDL_Texture* headTexture = nullptr;

    SDL_Rect srcRect{};
    SDL_Rect destRect{};

    bool animated = false;
    int frames = 0;
    int speed = 100;

    int frameWidth = 0;
    int frameHeight = 0;
    int scale = 1;

    bool hasHead = false;
    int headIndex = 0;          // Qué cabeza elegimos.
    int headFrameWidth = 16;    // Tamaño del recorte.
    int headFrameHeight = 16;
    int headStartX = 7;         // Donde empieza la primera cabeza.
    int headStartY = 14;        // Donde empieza la primera fila de cabezas.
    int headStepX = 27;         // Distancia entre una cabeza y otra.
    int headStepY = 64;         // Distancia entre una dirección y otra.
    std::string currentAnim = "";
};



#endif //PRUEBA_SDL_SPRITECOMPONENT_H

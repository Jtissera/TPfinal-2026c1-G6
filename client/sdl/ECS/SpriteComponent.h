
#ifndef PRUEBA_SDL_SPRITECOMPONENT_H
#define PRUEBA_SDL_SPRITECOMPONENT_H

#include <map>
#include <string>
#include "SDL2/SDL.h"
#include "ECS.h"
#include "Animation.h"
#include "TransformComponent.h"

class SpriteComponent : public Component {
public:
    int animationIndex = 0;
    SDL_RendererFlip spriteFlip = SDL_FLIP_NONE;
    std::map<const char*, Animation> animations;

    SpriteComponent() = default;
    explicit SpriteComponent(const char* path);
    SpriteComponent(const std::string& id, bool isAnimated);
    ~SpriteComponent() = default;

    void setText(const std::string& id);
    void Play(const char* animName);

    void init() override;
    void update() override;
    void draw() override;

private:
    TransformComponent* transform = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_Rect srcRect{};
    SDL_Rect destRect{};
    bool animated = false;
    int frames = 0;
    int speed = 100;
};



#endif //PRUEBA_SDL_SPRITECOMPONENT_H

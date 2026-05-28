
#ifndef PRUEBA_SDL_SPRITECOMPONENT_H
#define PRUEBA_SDL_SPRITECOMPONENT_H

#include <map>
#include <string>
#include "SDL2/SDL.h"
#include "ECS.h"
#include "Animation.h"
#include "SpriteSheetConfig.h"
#include "TransformComponent.h"

class AssetManager;
class TextureManager;

class SpriteComponent : public Component {
public:

    SDL_RendererFlip spriteFlip = SDL_FLIP_NONE;


    SpriteComponent(
        AssetManager& assets,
        const std::string& id,
        bool isAnimated,
        std::map<std::string, Animation> anims,
        SpriteSheetConfig config
    );

    ~SpriteComponent() = default;

    void setText(const std::string& id);
    void setHeadTexture(const std::string& textureId, int selectedHeadIndex);

    void Play(const char* animName);

    void init() override;
    void update(UpdateContext& context) override;
    void draw(RenderContext& context) override;

private:


    AssetManager& assets;
    TransformComponent* transform = nullptr;
    std::map<std::string, Animation> animations;
    int animationIndex = 0;

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
    int startX = 0;
    int startY = 0;

    bool hasHead = false;

    int headIndex = 0;
    int headFrameWidth = 16;
    int headFrameHeight = 16;
    int headStartX = 7;
    int headStartY = 14;
    int headStepX = 27;
    int headStepY = 64;

    std::string currentAnim = "";
    SpriteSheetConfig bodyConfigForRace(const std::string& race) const;
};

#endif
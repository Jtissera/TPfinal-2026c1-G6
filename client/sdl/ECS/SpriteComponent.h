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
    const SDL_Rect& getSrcRect() const;
    const SDL_Rect& getDestRect() const;

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
    void PlayOnce(const char* animName, const std::string& returnAnim);
    void setAttackTexture(const std::string& id);

    void init() override;
    void update(UpdateContext& context) override;
    void draw(RenderContext& context) override;

    int getStartX() const;
    int getStartY() const;
    int getAnimationIndex() const { return animationIndex; }

    void setSpriteTextureAndConfig(const std::string& newTextureId, const SpriteSheetConfig& newConfig);
    void setRenderOffset(int offsetX, int offsetY);
    void setHelmetTexture(const std::string& textureId,
        int offsetX, int offsetY,
        int srcW, int srcH,
        int downSrcX, int downSrcY,
        int leftSrcX, int leftSrcY,
        int rightSrcX, int rightSrcY,
        int upSrcX, int upSrcY);
    void clearHelmet();
    void setBody(const std::string& textureId, const SpriteSheetConfig& config);
    void clearHead();
    void setManualAnimation(bool manual);
    void StepFrame();

    // attack
    SDL_Texture* attackTexture = nullptr;
    int attackFrameWidth;
    int attackFrameHeight;
    bool usingAttackTexture = false;
    int animStartX = 0;

    // one-shot
    bool isOneShot = false;
    std::string oneShotReturnAnim = "IdleDown";
    Uint32 oneShotEndTime = 0;

    void setAttackTexture(const std::string &id, const SpriteSheetConfig &config);

private:
    bool isManualAnimation = false;
    int manualFrameIndex = 0;

    AssetManager& assets;
    TransformComponent* transform = nullptr;
    std::map<std::string, Animation> animations;
    int animationIndex = 0;

    SDL_Texture* bodyTexture = nullptr;
    SDL_Texture* headTexture = nullptr;
    SpriteSheetConfig attackConfig{};

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
    int renderOffsetX = 0;
    int renderOffsetY = 0;

    SDL_Texture* helmetTexture = nullptr;
    bool hasHelmet = false;
    int helmetOffsetX = 0;
    int helmetOffsetY = 0;
    int helmetSrcX = 0;
    int helmetSrcY = 0;
    int helmetSrcW = 32;
    int helmetSrcH = 32;
    int helmetDownSrcX = 0;
    int helmetDownSrcY = 0;
    int helmetLeftSrcX = 0;
    int helmetLeftSrcY = 0;
    int helmetRightSrcX = 0;
    int helmetRightSrcY = 0;
    int helmetUpSrcX = 0;
    int helmetUpSrcY = 0;

    std::string currentAnim = "";
    
};

#endif
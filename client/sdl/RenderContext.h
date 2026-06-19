
#ifndef TALLER_TP_RENDERCONTEXT_H
#define TALLER_TP_RENDERCONTEXT_H
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>


class TextureManager;
class AssetManager;

struct RenderContext {
    SDL_Renderer* renderer;
    SDL_Rect camera;
    SDL_Rect viewport;
    TextureManager& textureManager;
    AssetManager& assets;
    int mapOffsetY = 133;
};
#endif //TALLER_TP_RENDERCONTEXT_H

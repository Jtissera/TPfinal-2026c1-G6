
#ifndef TALLER_TP_RENDERCONTEXT_H
#define TALLER_TP_RENDERCONTEXT_H
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>


class TextureManager;

struct RenderContext {
    SDL_Renderer* renderer;          // Renderer usado para dibujar cosas directas de SDL.
    SDL_Rect camera;                 // Cámara actual.
    TextureManager& textureManager;  // Usado para dibujar texturas.
    int mapOffsetY = 133;            // Offset vertical del área jugable por el HUD.
};
#endif //TALLER_TP_RENDERCONTEXT_H

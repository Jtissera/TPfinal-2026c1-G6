#pragma once

#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "Screen.h"

class MainMenuScreen : public Screen {
public:
    // renderer debe vivir al menos mientras exista esta pantalla.
    // fontPath: ruta a un .ttf (ej. "assets/sprites/MapAssets/arial.ttf")
    MainMenuScreen(SDL_Renderer* renderer, int windowW, int windowH,
                   const std::string& fontPath);
    ~MainMenuScreen() override;

    ScreenResult run() override;

    // Permite inyectar un mensaje de error desde afuera (ej. fallo de conexión)
    void setError(const std::string& msg);

private:
    // ── Renderizado ──────────────────────────────────────────────────────────
    void render();
    void renderTitle();
    void renderMenuItems();
    void renderError();

    // ── Input ────────────────────────────────────────────────────────────────
    // Devuelve true + result si el usuario confirmó una acción
    bool handleEvent(const SDL_Event& e, ScreenResult& out);
    void moveSelection(int delta);

    // ── Helpers ──────────────────────────────────────────────────────────────
    SDL_Texture* makeTextTexture(const std::string& text, TTF_Font* font,
                                 SDL_Color color, SDL_Rect& outRect);
    void drawTexture(SDL_Texture* tex, const SDL_Rect& dst);
    void drawRect(const SDL_Rect& r, SDL_Color color, bool fill = true);

    // ── Estado ───────────────────────────────────────────────────────────────
    SDL_Renderer* renderer   = nullptr;
    TTF_Font*     fontLarge  = nullptr;
    TTF_Font*     fontMedium = nullptr;
    TTF_Font*     fontSmall  = nullptr;

    int windowW;
    int windowH;

    struct MenuItem {
        std::string  label;
        ScreenResult result;
    };
    std::vector<MenuItem> items;
    int selectedIndex = 0;

    std::string errorMsg;

    // ── Colores ──────────────────────────────────────────────────────────────
    static constexpr SDL_Color COLOR_BG         = {15,  12,  30,  255};
    static constexpr SDL_Color COLOR_TITLE       = {220, 180, 80,  255};
    static constexpr SDL_Color COLOR_ITEM        = {200, 200, 200, 255};
    static constexpr SDL_Color COLOR_SELECTED    = {255, 230, 100, 255};
    static constexpr SDL_Color COLOR_SELECTED_BG = {60,  45,  15,  120};
    static constexpr SDL_Color COLOR_ERROR       = {220, 60,  60,  255};
    static constexpr SDL_Color COLOR_FOOTER      = {100, 100, 100, 255};
};

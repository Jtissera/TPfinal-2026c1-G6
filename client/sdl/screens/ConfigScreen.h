#pragma once

#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include "Screen.h"

// Configuración persistida en ~/.config/argentum/client.toml
struct ClientConfig {
    int  musicVolume  = 70;   // 0-100
    int  sfxVolume    = 80;   // 0-100
    bool fullscreen   = false;
    int  resWidth     = 1080;
    int  resHeight    = 640;

    static ClientConfig load();   // carga desde disco (defaults si no existe)
    void save() const;            // persiste en ~/.config/argentum/client.toml
};

// AR-80 — Pantalla de configuración
// Controla: volumen música, volumen efectos, resolución, fullscreen.
// Los cambios se aplican en runtime y se persisten en client.toml.
// Recibe SDL_Window* para poder aplicar fullscreen/resolución sin reiniciar.
class ConfigScreen : public Screen {
public:
    ConfigScreen(SDL_Renderer* renderer, SDL_Window* window,
                 int windowW, int windowH,
                 const std::string& fontPath,
                 ClientConfig& config);
    ~ConfigScreen() override;

    ScreenResult run() override;

private:
    // ------------------ Render ------------------
    void render();
    void renderBackground();
    void renderPanel();
    void renderHeader();
    void renderOptions();
    void renderFooter();

    // ------------------ Helpers de dibujo ------------------
    void drawSlider(int x, int y, int w, int value, bool selected, bool hovered);
    void drawToggle(int x, int y, bool value, bool selected, bool hovered);
    void drawResOption(int x, int y, int rw, int rh, bool active, bool hovered);
    void drawFilledRoundRect(const SDL_Rect& r, SDL_Color c, int radius = 6);
    void drawBorderRoundRect(const SDL_Rect& r, SDL_Color c, int radius = 6);
    SDL_Texture* makeText(const std::string& text, TTF_Font* font,
                          SDL_Color color, SDL_Rect& out);
    void drawTex(SDL_Texture* tex, int x, int y);
    void drawRect(const SDL_Rect& r, SDL_Color color, bool fill = true);

    // ------------------ Input ------------------
    bool handleEvent(const SDL_Event& e, ScreenResult& out);
    void applyConfig();  // aplica cambios en runtime

    // ------------------ Estado ------------------
    SDL_Renderer* renderer;
    SDL_Window*   window;
    int windowW, windowH;

    TTF_Font* fontTitle  = nullptr;
    TTF_Font* fontMedium = nullptr;
    TTF_Font* fontSmall  = nullptr;

    ClientConfig& config;      // referencia al config que vive en Client
    ClientConfig  original;    // copia para cancelar

    // Opciones de resolucion disponibles
    struct Resolution { int w, h; };
    static constexpr Resolution RESOLUTIONS[] = {
        {1080, 640}, {1280, 720}, {1600, 900}, {1920, 1080}
    };
    static constexpr int RES_COUNT = 4;
    int selectedRes = 0;   // índice en RESOLUTIONS

    // Seleccion de fila (0=música, 1=sfx, 2=fullscreen, 3=resolución, 4=guardar, 5=cancelar)
    int selectedRow = 0;
    static constexpr int ROW_MUSIC    = 0;
    static constexpr int ROW_SFX      = 1;
    static constexpr int ROW_FULLSCR  = 2;
    static constexpr int ROW_RES      = 3;
    static constexpr int ROW_SAVE     = 4;
    static constexpr int ROW_CANCEL   = 5;
    static constexpr int ROW_COUNT    = 6;

    bool hoveredSave   = false;
    bool hoveredCancel = false;

    // Layout
    SDL_Rect panel{};
    int optStartY = 0;
    static constexpr int ROW_H    = 52;
    static constexpr int ROW_GAP  = 8;

    void computeLayout();

    // ------------------ Paleta ------------------
    static constexpr SDL_Color C_BG         = {15,  12,  30,  255};
    static constexpr SDL_Color C_PANEL      = {18,  15,  40,  230};
    static constexpr SDL_Color C_PANEL_BORD = {55,  45,  100, 255};
    static constexpr SDL_Color C_TITLE      = {220, 185, 80,  255};
    static constexpr SDL_Color C_SUBTITLE   = {140, 120, 60,  255};
    static constexpr SDL_Color C_TEXT       = {215, 215, 215, 255};
    static constexpr SDL_Color C_DIM        = {110, 105, 130, 255};
    static constexpr SDL_Color C_SEL        = {255, 230, 100, 255};
    static constexpr SDL_Color C_SEL_BG     = {55,  42,  12,  180};
    static constexpr SDL_Color C_HOV        = {255, 240, 150, 255};
    static constexpr SDL_Color C_DIVIDER    = {55,  45,  100, 180};
    static constexpr SDL_Color C_SLIDER_BG  = {30,  25,  65,  255};
    static constexpr SDL_Color C_SLIDER_FG  = {80,  160, 90,  255};
    static constexpr SDL_Color C_SLIDER_SEL = {120, 210, 130, 255};
    static constexpr SDL_Color C_ON        = {70,  160, 90,  255};
    static constexpr SDL_Color C_OFF       = {160, 55,  55,  255};
    static constexpr SDL_Color C_BTN_SAVE  = {30,  100, 50,  210};
    static constexpr SDL_Color C_BTN_CANC  = {90,  25,  25,  210};
};

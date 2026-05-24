#pragma once

#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "Screen.h"
#include "../../../common/dtos/gameTypes.h"

class CreateCharScreen : public Screen {
public:
    enum class Mode { CREATE, LOGIN };

    CreateCharScreen(SDL_Renderer* renderer, int windowW, int windowH,
                     const std::string& fontPath, Mode mode = Mode::CREATE);
    ~CreateCharScreen() override;

    ScreenResult run() override;

    const std::string& getUsername() const { return username; }
    Raza  getRaza()  const { return selectedRaza; }
    Clase getClase() const { return selectedClase; }

    void setError(const std::string& msg);

private:
    enum class Focus { NAME, RAZA, CLASE, CONFIRM, BACK };
    void render();
    void renderTitle();
    void renderNameField();
    void renderSelector(const char* label, const std::vector<std::string>& opts,
                        int selected, int x, int y, int w, bool active);
    void renderButtons();
    void renderError();
    bool handleEvent(const SDL_Event& e, ScreenResult& out);
    void handleKeyDown(SDL_Keycode key, ScreenResult& out, bool& done);
    void handleTextInput(const char* text);
    void handleBackspace();
    void cycleFocus(int delta);
    void cycleOption(int delta);
    ScreenResult confirm();
    SDL_Texture* makeText(const std::string& text, TTF_Font* font,
                          SDL_Color color, SDL_Rect& out);
    void drawTex(SDL_Texture* tex, const SDL_Rect& dst);
    void drawRect(const SDL_Rect& r, SDL_Color color, bool fill = true);
    bool isClickOn(const SDL_Rect& r, int mx, int my) const;
    SDL_Renderer* renderer   = nullptr;
    TTF_Font*     fontLarge  = nullptr;
    TTF_Font*     fontMedium = nullptr;
    TTF_Font*     fontSmall  = nullptr;

    int   windowW, windowH;
    Mode  mode;
    Focus focus = Focus::NAME;

    std::string username;
    int razaIdx  = 0;
    int claseIdx = 0;

    Raza  selectedRaza  = Raza::HUMANO;
    Clase selectedClase = Clase::MAGO;

    std::string errorMsg;

    static const std::vector<std::string> RAZAS;
    static const std::vector<std::string> CLASES;

    static constexpr SDL_Color C_BG       = {15,  12,  30,  255};
    static constexpr SDL_Color C_TITLE    = {220, 180, 80,  255};
    static constexpr SDL_Color C_LABEL    = {160, 160, 160, 255};
    static constexpr SDL_Color C_TEXT     = {230, 230, 230, 255};
    static constexpr SDL_Color C_ACTIVE   = {255, 230, 100, 255};
    static constexpr SDL_Color C_ACTIVE_BG= {60,  45,  15,  140};
    static constexpr SDL_Color C_INACTIVE = {120, 120, 120, 255};
    static constexpr SDL_Color C_CURSOR   = {255, 230, 100, 255};
    static constexpr SDL_Color C_ERROR    = {220, 60,  60,  255};
    static constexpr SDL_Color C_BTN_OK   = {40,  140, 60,  200};
    static constexpr SDL_Color C_BTN_BACK = {100, 40,  40,  200};
};

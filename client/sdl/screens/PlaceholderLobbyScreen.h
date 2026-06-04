#pragma once

#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "Screen.h"
#include "../../network/clientProtocolFactory.h"
#include "../../../common/network/messages/server/lobby/gameListMessage.h"
#include "../../../common/dtos/gameTypes.h"

// AR-79 — LobbyScreen
// Pantalla de lobby visual completa: lista de partidas, crear partida, unirse.
class PlaceholderLobbyScreen : public Screen {
public:
    PlaceholderLobbyScreen(SDL_Renderer* renderer, int windowW, int windowH,
                            const std::string& fontPath,
                            Protocol& protocol,
                            const std::string& username);
    ~PlaceholderLobbyScreen() override;

    ScreenResult run() override;
    const PlayerDto& getJoinedPlayerDto() const;
private:
    // ----------------- Red -----------------
    void fetchGameList();
    void tryCreateGame();
    void tryJoinSelected();

    // ----------------- Loop / input -----------------
    void render();
    bool handleEvent(const SDL_Event& e, ScreenResult& out);

    // ----------------- Render helpers -----------------
    void renderBackground();
    void renderPanel();
    void renderHeader();
    void renderGameList();
    void renderCreateSection();
    void renderStatusBar();

    void drawFilledRoundRect(const SDL_Rect& r, SDL_Color color, int radius = 6);
    void drawBorderRoundRect(const SDL_Rect& r, SDL_Color color, int radius = 6);
    void drawButton(const SDL_Rect& r, const char* label,
                    SDL_Color bg, SDL_Color border, bool hovered);

    SDL_Texture* makeText(const std::string& text, TTF_Font* font,
                          SDL_Color color, SDL_Rect& out);
    void         drawTex(SDL_Texture* tex, const SDL_Rect& dst);
    void         drawRect(const SDL_Rect& r, SDL_Color color, bool fill = true);

    // ----------------- Estado -----------------
    SDL_Renderer* renderer;
    TTF_Font*     fontTitle  = nullptr;
    TTF_Font*     fontMedium = nullptr;
    TTF_Font*     fontSmall  = nullptr;
    int windowW, windowH;

    Protocol&   protocol;
    std::string username;

    struct GameEntry {
        uint32_t    id;
        std::string name;
        uint8_t     players;
        uint8_t     maxPlayers;
    };
    std::vector<GameEntry> games;
    int selectedGame  = -1;
    int hoveredGame   = -1;

    std::string newGameName;
    bool        typingNewGame = false;

    std::string errorMsg;
    std::string statusMsg;
    bool        _readyToPlay = false;

    // DTO real del jugador recibido desde el servidor al entrar a una partida.
    PlayerDto joinedPlayerDto{};

    // Hover sobre botones principales
    bool hoverJoin    = false;
    bool hoverRefresh = false;
    bool hoverBack    = false;
    bool hoverCreate  = false;

    // Layout — calculado en el constructor
    SDL_Rect panel{};       // panel central
    SDL_Rect listArea{};    // zona de la lista de partidas
    SDL_Rect createArea{};  // zona de crear partida
    SDL_Rect buttonArea{};  // zona de botones principales

    void computeLayout();

    // ─-----------------─ Paleta -----------------
    static constexpr SDL_Color C_BG         = {10,  8,   22,  255};
    static constexpr SDL_Color C_PANEL      = {18,  15,  40,  230};
    static constexpr SDL_Color C_PANEL_BORD = {55,  45,  100, 255};
    static constexpr SDL_Color C_TITLE      = {220, 185, 80,  255};
    static constexpr SDL_Color C_SUBTITLE   = {140, 120, 60,  255};
    static constexpr SDL_Color C_TEXT       = {215, 215, 215, 255};
    static constexpr SDL_Color C_DIM        = {110, 105, 130, 255};
    static constexpr SDL_Color C_SEL        = {255, 230, 100, 255};
    static constexpr SDL_Color C_SEL_BG     = {55,  42,  12,  180};
    static constexpr SDL_Color C_HOV        = {255, 240, 150, 255};
    static constexpr SDL_Color C_HOV_BG     = {40,  32,  8,   120};
    static constexpr SDL_Color C_ROW_EVEN   = {22,  18,  48,  200};
    static constexpr SDL_Color C_ROW_ODD    = {26,  22,  55,  200};
    static constexpr SDL_Color C_ROW_BORD   = {50,  44,  85,  255};
    static constexpr SDL_Color C_FULL       = {160, 55,  55,  255};
    static constexpr SDL_Color C_AVAIL      = {70,  160, 90,  255};
    static constexpr SDL_Color C_INPUT_BG   = {20,  16,  45,  220};
    static constexpr SDL_Color C_INPUT_ACT  = {40,  32,  10,  220};
    static constexpr SDL_Color C_BTN_JOIN   = {30,  70,  130, 210};
    static constexpr SDL_Color C_BTN_REF    = {55,  55,  25,  210};
    static constexpr SDL_Color C_BTN_BACK   = {90,  25,  25,  210};
    static constexpr SDL_Color C_BTN_CRE    = {30,  100, 50,  210};
    static constexpr SDL_Color C_ERROR      = {220, 65,  65,  255};
    static constexpr SDL_Color C_STATUS     = {100, 200, 120, 255};
    static constexpr SDL_Color C_DIVIDER    = {55,  45,  100, 180};
};

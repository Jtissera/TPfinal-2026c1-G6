#pragma once

#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "Screen.h"
#include "../../network/clientProtocolFactory.h"
#include "../../../common/network/messages/server/lobby/gameListMessage.h"

// Lobby SDL mínimo — placeholder hasta AR-79.
class PlaceholderLobbyScreen : public Screen {
public:
    PlaceholderLobbyScreen(SDL_Renderer* renderer, int windowW, int windowH,
                            const std::string& fontPath,
                            Protocol& protocol,
                            const std::string& username);
    ~PlaceholderLobbyScreen() override;

    ScreenResult run() override;

private:
    void fetchGameList();
    void tryCreateGame();
    void tryJoinSelected();
    void render();
    void renderTitle();
    void renderGameList();
    void renderInput();
    void renderButtons();
    void renderError();
    bool handleEvent(const SDL_Event& e, ScreenResult& out);

    SDL_Texture* makeText(const std::string& text, TTF_Font* font,
                          SDL_Color color, SDL_Rect& out);
    void drawTex(SDL_Texture* tex, const SDL_Rect& dst);
    void drawRect(const SDL_Rect& r, SDL_Color color, bool fill = true);

    SDL_Renderer* renderer;
    TTF_Font* fontMedium = nullptr;
    TTF_Font* fontSmall  = nullptr;
    int windowW, windowH;

    Protocol& protocol;
    std::string username;

    struct GameEntry {
        uint32_t    id;
        std::string name;
        uint8_t     players;
        uint8_t     maxPlayers;
    };
    std::vector<GameEntry> games;
    int selectedGame = -1;

    std::string newGameName;
    bool typingNewGame = false;

    std::string errorMsg;
    bool _readyToPlay = false;

    static constexpr SDL_Color C_BG      = {15,  12,  30,  255};
    static constexpr SDL_Color C_TITLE   = {220, 180, 80,  255};
    static constexpr SDL_Color C_TEXT    = {210, 210, 210, 255};
    static constexpr SDL_Color C_SEL     = {255, 230, 100, 255};
    static constexpr SDL_Color C_SEL_BG  = {60,  45,  15,  140};
    static constexpr SDL_Color C_ERROR   = {220, 60,  60,  255};
    static constexpr SDL_Color C_DIM     = {100, 100, 100, 255};
};

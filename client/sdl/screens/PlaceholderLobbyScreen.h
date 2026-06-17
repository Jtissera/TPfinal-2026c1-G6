#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include <optional>

#include "Screen.h"
#include "../../../common/network/protocol/protocol.h"
#include "../../../common/network/messages/server/lobby/gameListMessage.h"
#include "../../../common/dtos/gameTypes.h"
#include "../../../common/network/messages/server/system/mapChangedMessage.h"

struct MapEntry
{
    std::string path;
    std::string name;
    uint16_t width = 0;
    uint16_t height = 0;
};

struct GameEntry
{
    uint32_t id;
    std::string name;
    int players;
    int maxPlayers;
    std::string mapPath;
};

class PlaceholderLobbyScreen
{
public:
    PlaceholderLobbyScreen(SDL_Renderer *, int windowW, int windowH,
                           const std::string &fontPath,
                           Protocol &, const std::string &username);
    ~PlaceholderLobbyScreen();

    ScreenResult run();
    const PlayerDto &getJoinedPlayerDto() const;

    std::string getChosenMapPath() const { return chosenMapPath; }
    std::shared_ptr<const Message> takePendingMessage() { return std::move(pendingMessage); }

private:
    std::shared_ptr<const Message> pendingMessage = nullptr;
    
    int gamesScrollOffset = 0;
    int hoveredGame = -1;

    std::string chosenMapPath;
    SDL_Renderer *renderer;
    int windowW, windowH;
    TTF_Font *fontTitle = nullptr, *fontMedium = nullptr, *fontSmall = nullptr;

    Protocol &protocol;
    std::string username;
    PlayerDto joinedPlayerDto{};
    bool _readyToPlay = false;

    std::vector<GameEntry> games;
    int selectedGame = -1;

    std::string newGameName;
    bool typingNewGame = false;

    std::vector<MapEntry> maps;
    int selectedMap = 0;
    int hoveredMap = -1;
    int mapScrollOffset = 0;

    std::string errorMsg;
    std::string statusMsg;

    SDL_Rect panel{}, listArea{}, createRow{}, mapsArea{}, buttonArea{};
    SDL_Rect mapListArea{};

    bool hoverJoin = false, hoverRefresh = false, hoverBack = false, hoverCreate = false;

    void computeLayout();

    void fetchGameList();
    void tryCreateGame();
    void tryJoinSelected();

    void scanMaps();

    // Scroll / paginado de listas
    void clampScrollOffsets();
    int visibleGamesCount() const;
    int visibleMapsCount() const;

    bool handleEvent(const SDL_Event &, ScreenResult &);
    void render();
    void renderBackground();
    void renderPanel();
    void renderHeader();
    void renderGameList();
    void renderCreateSection();
    void renderMapList();
    void renderStatusBar();

    void drawFilledRoundRect(const SDL_Rect &, SDL_Color, int radius = 6);
    void drawBorderRoundRect(const SDL_Rect &, SDL_Color, int radius = 6);
    void drawButton(const SDL_Rect &, const char *label,
                    SDL_Color bg, SDL_Color border, bool hovered);
    SDL_Texture *makeText(const std::string &, TTF_Font *, SDL_Color, SDL_Rect &out);
    void drawTex(SDL_Texture *, const SDL_Rect &);
    void drawRect(const SDL_Rect &, SDL_Color, bool fill);

    // ── Layout de listas (partidas y mapas) ─────────────────────────────
    static constexpr int SCROLLBAR_W = 6;
    static constexpr int SCROLLBAR_GAP = 6;

    static constexpr int GAME_ROW_H = 36;
    static constexpr int GAME_ROW_SP = 4;
    static constexpr int GAME_LIST_HEADER_GAP = 34;
    static constexpr int GAME_LIST_BOTTOM_PAD = 8;

    static constexpr int MAP_ROW_H = 44;
    static constexpr int MAP_ROW_SP = 6;
    static constexpr int MAP_LIST_HEADER_GAP = 34;
    static constexpr int MAP_LIST_BOTTOM_PAD = 8;

    static constexpr int CREATE_INPUT_Y_OFFSET = 32;

    // ── Paleta ───────────────────────────────────────────────────────────
    static constexpr SDL_Color C_BG = {18, 18, 35, 255};
    static constexpr SDL_Color C_PANEL = {28, 28, 50, 245};
    static constexpr SDL_Color C_PANEL_BORD = {80, 60, 140, 200};
    static constexpr SDL_Color C_TITLE = {220, 200, 100, 255};
    static constexpr SDL_Color C_SUBTITLE = {140, 120, 200, 255};
    static constexpr SDL_Color C_TEXT = {220, 220, 220, 255};
    static constexpr SDL_Color C_DIM = {130, 120, 150, 255};
    static constexpr SDL_Color C_DIVIDER = {60, 50, 90, 180};
    static constexpr SDL_Color C_SEL = {100, 160, 255, 255};
    static constexpr SDL_Color C_SEL_BG = {30, 50, 90, 200};
    static constexpr SDL_Color C_HOV = {180, 200, 255, 255};
    static constexpr SDL_Color C_HOV_BG = {40, 40, 70, 180};
    static constexpr SDL_Color C_ROW_EVEN = {32, 30, 52, 220};
    static constexpr SDL_Color C_ROW_ODD = {26, 24, 44, 220};
    static constexpr SDL_Color C_ROW_BORD = {55, 45, 85, 160};
    static constexpr SDL_Color C_FULL = {220, 80, 80, 255};
    static constexpr SDL_Color C_AVAIL = {80, 200, 100, 255};
    static constexpr SDL_Color C_ERROR = {240, 80, 80, 255};
    static constexpr SDL_Color C_STATUS = {100, 200, 120, 255};
    static constexpr SDL_Color C_INPUT_BG = {22, 22, 40, 255};
    static constexpr SDL_Color C_INPUT_ACT = {28, 28, 55, 255};
    static constexpr SDL_Color C_BTN_JOIN = {30, 60, 110, 255};
    static constexpr SDL_Color C_BTN_REF = {70, 70, 30, 255};
    static constexpr SDL_Color C_BTN_BACK = {90, 30, 30, 255};
    static constexpr SDL_Color C_BTN_CRE = {30, 80, 50, 255};
    static constexpr SDL_Color C_MAP_SEL = {60, 100, 200, 255};
    static constexpr SDL_Color C_MAP_SEL_BG = {20, 40, 80, 220};
};
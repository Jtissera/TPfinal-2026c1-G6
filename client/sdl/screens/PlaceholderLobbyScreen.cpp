#include "PlaceholderLobbyScreen.h"

#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstring>

#include "../../../common/network/messages/client/lobby/listGamesMessage.h"
#include "../../../common/network/messages/client/lobby/createGameMessage.h"
#include "../../../common/network/messages/client/lobby/joinGameMessage.h"
#include "../../../common/network/messages/server/lobby/gameCreatedMessage.h"
#include "../../../common/network/messages/server/lobby/joinOkMessage.h"
#include "../../../common/network/messages/server/error/errorMessage.h"
#include "../../../common/network/protocol/serverOpCode.h"

namespace fs = std::filesystem;
namespace
{

    static constexpr uint8_t ARGMAP_MAGIC[8] =
        {'A', 'R', 'G', 'M', 'A', 'P', 0, 0};

    static uint16_t readU16LE(std::istream &is)
    {
        uint8_t lo = 0, hi = 0;
        is.read(reinterpret_cast<char *>(&lo), 1);
        is.read(reinterpret_cast<char *>(&hi), 1);
        return static_cast<uint16_t>(lo | (hi << 8));
    }

    struct MapHeader
    {
        std::string name;
        uint16_t w = 0, h = 0;
        bool ok = false;
    };

    MapHeader readArgmapHeader(const std::string &path)
    {
        std::ifstream f(path, std::ios::binary);
        if (!f)
            return {};

        uint8_t magic[8] = {};
        f.read(reinterpret_cast<char *>(magic), 8);
        if (std::memcmp(magic, ARGMAP_MAGIC, 8) != 0)
            return {};

        uint16_t version = readU16LE(f);
        if (version < 1 || version > 10)
            return {};

        if (version >= 3)
            f.get();

        uint16_t w = readU16LE(f);
        uint16_t h = readU16LE(f);

        uint16_t nameLen = readU16LE(f);
        if (nameLen > 256)
            return {};
        f.seekg(nameLen, std::ios::cur);

        MapHeader hdr;
        hdr.name = fs::path(path).stem().string();
        hdr.w = w;
        hdr.h = h;
        hdr.ok = f.good();
        return hdr;
    }

}

PlaceholderLobbyScreen::PlaceholderLobbyScreen(SDL_Renderer *renderer,
                                               int windowW, int windowH,
                                               const std::string &fontPath,
                                               Protocol &protocol,
                                               const std::string &username)
    : renderer(renderer), windowW(windowW), windowH(windowH),
      protocol(protocol), username(username)
{
    if (TTF_WasInit() == 0 && TTF_Init() == -1)
        throw std::runtime_error(std::string("TTF_Init: ") + TTF_GetError());

    fontTitle = TTF_OpenFont(fontPath.c_str(), 32);
    fontMedium = TTF_OpenFont(fontPath.c_str(), 20);
    fontSmall = TTF_OpenFont(fontPath.c_str(), 15);

    if (!fontTitle || !fontMedium || !fontSmall)
        throw std::runtime_error(std::string("TTF_OpenFont: ") + TTF_GetError());

    scanMaps();
    computeLayout();
    SDL_StartTextInput();
    fetchGameList();
}

PlaceholderLobbyScreen::~PlaceholderLobbyScreen()
{
    SDL_StopTextInput();
    if (fontTitle)
    {
        TTF_CloseFont(fontTitle);
        fontTitle = nullptr;
    }
    if (fontMedium)
    {
        TTF_CloseFont(fontMedium);
        fontMedium = nullptr;
    }
    if (fontSmall)
    {
        TTF_CloseFont(fontSmall);
        fontSmall = nullptr;
    }
}

void PlaceholderLobbyScreen::scanMaps()
{
    maps.clear();
    const std::string worldsDir = "assets/sprites/MapAssets/worlds";

    std::error_code ec;
    if (!fs::exists(worldsDir, ec) || !fs::is_directory(worldsDir, ec))
    {
        std::cerr << "[LobbyScreen] Carpeta de mapas no encontrada: "
                  << worldsDir << std::endl;
        return;
    }

    std::vector<fs::path> files;
    for (const auto &entry : fs::directory_iterator(worldsDir, ec))
        if (entry.path().extension() == ".argmap")
            files.push_back(entry.path());
    std::sort(files.begin(), files.end());

    for (const auto &p : files)
    {
        MapHeader hdr = readArgmapHeader(p.string());
        if (!hdr.ok)
        {
            std::cerr << "[LobbyScreen] No se pudo leer header de: "
                      << p.string() << std::endl;
            continue;
        }

        MapEntry entry;
        entry.path = p.string();
        entry.name = hdr.name;
        entry.width = hdr.w;
        entry.height = hdr.h;
        maps.push_back(std::move(entry));

        std::cout << "[LobbyScreen] mapa encontrado: '" << hdr.name
                  << "' " << hdr.w << "x" << hdr.h
                  << " path=" << p.string() << std::endl;
    }

    if (!maps.empty())
        selectedMap = 0;
}

void PlaceholderLobbyScreen::computeLayout()
{
    const int panelW = 860;
    const int panelH = windowH - 80;
    panel = {(windowW - panelW) / 2, 40, panelW, panelH};

    const int pad = 16;
    const int headerH = 80;
    const int btnH = 42;
    const int createH = 74;

    const int innerH = panelH - headerH - createH - btnH - pad * 5;

    const int listH = GAME_LIST_HEADER_GAP + (2 * (GAME_ROW_H + GAME_ROW_SP)) + 24;

    const int mapsH = innerH - listH;

    listArea = {panel.x + pad, panel.y + headerH + pad, panelW - pad * 2, listH};

    createRow = {panel.x + pad, listArea.y + listArea.h + pad, panelW - pad * 2, createH};

    mapsArea = {panel.x + pad, createRow.y + createRow.h + pad, panelW - pad * 2, mapsH};
    mapListArea = mapsArea;

    buttonArea = {panel.x + pad, panel.y + panelH - btnH - pad, panelW - pad * 2, btnH};
}

int PlaceholderLobbyScreen::visibleGamesCount() const
{
    return std::max(1, (listArea.h - GAME_LIST_HEADER_GAP - GAME_LIST_BOTTOM_PAD) /
                           (GAME_ROW_H + GAME_ROW_SP));
}

int PlaceholderLobbyScreen::visibleMapsCount() const
{
    return std::max(1, (mapListArea.h - MAP_LIST_HEADER_GAP - MAP_LIST_BOTTOM_PAD) /
                           (MAP_ROW_H + MAP_ROW_SP));
}

void PlaceholderLobbyScreen::clampScrollOffsets()
{
    const int maxGameScroll = std::max(0, static_cast<int>(games.size()) - visibleGamesCount());
    gamesScrollOffset = std::clamp(gamesScrollOffset, 0, maxGameScroll);

    const int maxMapScroll = std::max(0, static_cast<int>(maps.size()) - visibleMapsCount());
    mapScrollOffset = std::clamp(mapScrollOffset, 0, maxMapScroll);
}

void PlaceholderLobbyScreen::fetchGameList()
{
    try
    {
        protocol.send(ListGamesMessage());
        auto response = protocol.receive();
        if (response->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_GAME_LIST))
        {
            const auto &msg = static_cast<const GameListMessage &>(*response);
            games.clear();
            for (const auto &g : msg.getGames())
                games.push_back({g.gameId, g.gameName, g.playerCount, g.maxPlayers, g.mapPath});
            if (selectedGame >= static_cast<int>(games.size()))
                selectedGame = games.empty() ? -1 : 0;
            clampScrollOffsets();
            errorMsg.clear();
            statusMsg = "Lista actualizada.";
        }
    }
    catch (const std::exception &e)
    {
        errorMsg = std::string("Error al listar: ") + e.what();
        statusMsg.clear();
    }
}

void PlaceholderLobbyScreen::tryCreateGame()
{
    if (newGameName.empty())
    {
        errorMsg = "Ingresa un nombre para la partida.";
        return;
    }
    if (maps.empty())
    {
        errorMsg = "No hay mapas disponibles en assets/sprites/MapAssets/worlds/";
        return;
    }
    if (selectedMap < 0 || selectedMap >= static_cast<int>(maps.size()))
    {
        errorMsg = "Selecciona un mapa.";
        return;
    }

    const std::string &chosenMap = maps[selectedMap].path;

    try
    {
        protocol.send(CreateGameMessage(newGameName, 4, chosenMap));
        auto response = protocol.receive();
        if (response->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_GAME_CREATED))
        {
            const auto &created = static_cast<const GameCreatedMessage &>(*response);
            protocol.send(JoinGameMessage(created.getGameId()));
            auto joinResponse = protocol.receive();
            if (joinResponse->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_JOIN_OK))
            {
                const auto &joinOk = static_cast<const JoinOkMessage &>(*joinResponse);
                joinedPlayerDto = joinOk.getPlayerDto();
                this->chosenMapPath = chosenMap;
                std::cout
                    << "[LOBBY] partida creada con mapa='" << chosenMap << "'"
                    << " raza='" << joinedPlayerDto.raza
                    << "' clase='" << joinedPlayerDto.clase << "'" << std::endl;
                _readyToPlay = true;
            }
            else if (joinResponse->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_ERROR))
                errorMsg = static_cast<const ErrorMessage &>(*joinResponse).getReason();
        }
        else if (response->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_ERROR))
            errorMsg = static_cast<const ErrorMessage &>(*response).getReason();
    }
    catch (const std::exception &e)
    {
        errorMsg = std::string("Error: ") + e.what();
    }
}

void PlaceholderLobbyScreen::tryJoinSelected()
{
    if (selectedGame < 0 || selectedGame >= static_cast<int>(games.size()))
    {
        errorMsg = "Selecciona una partida primero.";
        return;
    }
    if (games[selectedGame].players >= games[selectedGame].maxPlayers)
    {
        errorMsg = "La partida está llena.";
        return;
    }
    try
    {
        protocol.send(JoinGameMessage(games[selectedGame].id));
        auto response = protocol.receive();
        if (response->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_JOIN_OK))
        {
            joinedPlayerDto =
                static_cast<const JoinOkMessage &>(*response).getPlayerDto();

            this->chosenMapPath = games[selectedGame].mapPath;

            // Si el jugador reconecta desde una instancia, el servidor manda
            // un MapChangedMessage inmediatamente después del JoinOk.
            // Lo consumimos acá para inicializar Game con el mapa correcto.
            auto next = protocol.receive();
            if (next->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_MAP_CHANGED))
            {
                const auto &mapMsg = static_cast<const MapChangedMessage &>(*next);
                this->chosenMapPath = mapMsg.getMapPath();
                std::cout << "[LOBBY] reconexión a instancia, mapa="
                          << this->chosenMapPath << std::endl;
            }
            else
            {
                // No era MapChanged — lo guardamos para que GameClient lo procese.
                pendingMessage = std::move(next);
            }

            _readyToPlay = true;
        }
        else if (response->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_ERROR))
            errorMsg = static_cast<const ErrorMessage &>(*response).getReason();
    }
    catch (const std::exception &e)
    {
        errorMsg = std::string("Error: ") + e.what();
    }
}

ScreenResult PlaceholderLobbyScreen::run()
{
    while (true)
    {
        if (_readyToPlay)
            return ScreenResult::GO_LOBBY;

        int mx, my;
        SDL_GetMouseState(&mx, &my);

        hoveredGame = -1;
        {
            const int rowW = listArea.w - SCROLLBAR_W - SCROLLBAR_GAP;
            const int startY = listArea.y + GAME_LIST_HEADER_GAP;
            const int visible = visibleGamesCount();
            for (int i = 0; i < visible; ++i)
            {
                int gameIdx = gamesScrollOffset + i;
                if (gameIdx >= static_cast<int>(games.size()))
                    break;
                SDL_Rect row = {listArea.x, startY + i * (GAME_ROW_H + GAME_ROW_SP), rowW, GAME_ROW_H};
                if (mx >= row.x && mx < row.x + row.w && my >= row.y && my < row.y + row.h)
                    hoveredGame = gameIdx;
            }
        }

        hoveredMap = -1;
        {
            const int rowW = mapListArea.w - SCROLLBAR_W - SCROLLBAR_GAP;
            const int startY = mapListArea.y + MAP_LIST_HEADER_GAP;
            const int visible = visibleMapsCount();
            for (int i = 0; i < visible; ++i)
            {
                int mapIdx = mapScrollOffset + i;
                if (mapIdx >= static_cast<int>(maps.size()))
                    break;
                SDL_Rect row = {mapListArea.x, startY + i * (MAP_ROW_H + MAP_ROW_SP), rowW, MAP_ROW_H};
                if (mx >= row.x && mx < row.x + row.w && my >= row.y && my < row.y + row.h)
                    hoveredMap = mapIdx;
            }
        }

        const int bw = (buttonArea.w - 20) / 3;
        SDL_Rect bJoin = {buttonArea.x, buttonArea.y, bw, buttonArea.h};
        SDL_Rect bRefresh = {buttonArea.x + bw + 10, buttonArea.y, bw, buttonArea.h};
        SDL_Rect bBack = {buttonArea.x + (bw + 10) * 2, buttonArea.y, bw, buttonArea.h};

        const int createBtnW = 110;
        SDL_Rect bCreate = {createRow.x + createRow.w - createBtnW,
                            createRow.y + CREATE_INPUT_Y_OFFSET,
                            createBtnW, 36};

        auto inRect = [&](SDL_Rect r)
        { return mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h; };

        hoverJoin = inRect(bJoin);
        hoverRefresh = inRect(bRefresh);
        hoverBack = inRect(bBack);
        hoverCreate = inRect(bCreate);

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                return ScreenResult::QUIT;

            ScreenResult res;
            if (handleEvent(e, res))
                return res;
        }

        render();
        SDL_RenderPresent(renderer);
        SDL_Delay(33);
    }
}

bool PlaceholderLobbyScreen::handleEvent(const SDL_Event &e, ScreenResult &out)
{
    if (e.type == SDL_KEYDOWN)
    {
        errorMsg.clear();
        statusMsg.clear();
        switch (e.key.keysym.sym)
        {
        case SDLK_ESCAPE:
            out = ScreenResult::GO_MAIN_MENU;
            return true;
        case SDLK_TAB:
            typingNewGame = !typingNewGame;
            break;
        case SDLK_BACKSPACE:
            if (typingNewGame && !newGameName.empty())
                newGameName.pop_back();
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            if (typingNewGame)
                tryCreateGame();
            else if (selectedGame >= 0)
                tryJoinSelected();
            break;
        case SDLK_UP:
        case SDLK_w:
            if (!games.empty())
            {
                selectedGame = (selectedGame <= 0)
                                   ? static_cast<int>(games.size()) - 1
                                   : selectedGame - 1;
                const int vis = visibleGamesCount();
                if (selectedGame < gamesScrollOffset)
                    gamesScrollOffset = selectedGame;
                else if (selectedGame >= gamesScrollOffset + vis)
                    gamesScrollOffset = selectedGame - vis + 1;
            }
            typingNewGame = false;
            break;
        case SDLK_DOWN:
        case SDLK_s:
            if (!games.empty())
            {
                selectedGame = (selectedGame + 1) % static_cast<int>(games.size());
                const int vis = visibleGamesCount();
                if (selectedGame < gamesScrollOffset)
                    gamesScrollOffset = selectedGame;
                else if (selectedGame >= gamesScrollOffset + vis)
                    gamesScrollOffset = selectedGame - vis + 1;
            }
            typingNewGame = false;
            break;
        case SDLK_PAGEUP:
            if (!typingNewGame && selectedMap > 0)
            {
                --selectedMap;
                if (selectedMap < mapScrollOffset)
                    mapScrollOffset = selectedMap;
            }
            break;
        case SDLK_PAGEDOWN:
            if (!typingNewGame && selectedMap + 1 < static_cast<int>(maps.size()))
            {
                ++selectedMap;
                const int vis = visibleMapsCount();
                if (selectedMap >= mapScrollOffset + vis)
                    mapScrollOffset = selectedMap - vis + 1;
            }
            break;
        case SDLK_F5:
            fetchGameList();
            break;
        default:
            break;
        }
        clampScrollOffsets();
    }

    if (e.type == SDL_TEXTINPUT && typingNewGame)
        if (newGameName.size() < 24)
            newGameName += e.text.text;

    if (e.type == SDL_MOUSEWHEEL)
    {
        int mx2, my2;
        SDL_GetMouseState(&mx2, &my2);

        if (mx2 >= mapListArea.x && mx2 < mapListArea.x + mapListArea.w &&
            my2 >= mapListArea.y && my2 < mapListArea.y + mapListArea.h)
        {
            mapScrollOffset -= e.wheel.y;
        }
        else if (mx2 >= listArea.x && mx2 < listArea.x + listArea.w &&
                 my2 >= listArea.y && my2 < listArea.y + listArea.h)
        {
            gamesScrollOffset -= e.wheel.y;
        }
        clampScrollOffsets();
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        int mx2 = e.button.x, my2 = e.button.y;
        errorMsg.clear();

        {
            const int rowW = listArea.w - SCROLLBAR_W - SCROLLBAR_GAP;
            const int startY = listArea.y + GAME_LIST_HEADER_GAP;
            const int visible = visibleGamesCount();
            for (int i = 0; i < visible; ++i)
            {
                int gameIdx = gamesScrollOffset + i;
                if (gameIdx >= static_cast<int>(games.size()))
                    break;
                SDL_Rect row = {listArea.x, startY + i * (GAME_ROW_H + GAME_ROW_SP), rowW, GAME_ROW_H};
                if (mx2 >= row.x && mx2 < row.x + row.w &&
                    my2 >= row.y && my2 < row.y + row.h)
                {
                    if (selectedGame == gameIdx)
                        tryJoinSelected();
                    else
                    {
                        selectedGame = gameIdx;
                        typingNewGame = false;
                    }
                }
            }
        }

        const int createBtnW = 110;
        SDL_Rect inputBox = {createRow.x,
                             createRow.y + CREATE_INPUT_Y_OFFSET,
                             createRow.w - createBtnW - 8, 36};
        if (mx2 >= inputBox.x && mx2 < inputBox.x + inputBox.w &&
            my2 >= inputBox.y && my2 < inputBox.y + inputBox.h)
            typingNewGame = true;

        SDL_Rect bCreate2 = {createRow.x + createRow.w - createBtnW,
                             createRow.y + CREATE_INPUT_Y_OFFSET,
                             createBtnW, 36};
        if (mx2 >= bCreate2.x && mx2 < bCreate2.x + bCreate2.w &&
            my2 >= bCreate2.y && my2 < bCreate2.y + bCreate2.h)
            tryCreateGame();

        {
            const int rowW = mapListArea.w - SCROLLBAR_W - SCROLLBAR_GAP;
            const int startY = mapListArea.y + MAP_LIST_HEADER_GAP;
            const int visible = visibleMapsCount();
            for (int i = 0; i < visible; ++i)
            {
                int mapIdx = mapScrollOffset + i;
                if (mapIdx >= static_cast<int>(maps.size()))
                    break;
                SDL_Rect row = {mapListArea.x, startY + i * (MAP_ROW_H + MAP_ROW_SP), rowW, MAP_ROW_H};
                if (mx2 >= row.x && mx2 < row.x + row.w &&
                    my2 >= row.y && my2 < row.y + row.h)
                {
                    selectedMap = mapIdx;
                    typingNewGame = false;
                }
            }
        }

        const int bw2 = (buttonArea.w - 20) / 3;
        SDL_Rect bJoin2 = {buttonArea.x, buttonArea.y, bw2, buttonArea.h};
        SDL_Rect bRefresh2 = {buttonArea.x + bw2 + 10, buttonArea.y, bw2, buttonArea.h};
        SDL_Rect bBack2 = {buttonArea.x + (bw2 + 10) * 2, buttonArea.y, bw2, buttonArea.h};

        if (mx2 >= bJoin2.x && mx2 < bJoin2.x + bJoin2.w &&
            my2 >= bJoin2.y && my2 < bJoin2.y + bJoin2.h)
            tryJoinSelected();

        if (mx2 >= bRefresh2.x && mx2 < bRefresh2.x + bRefresh2.w &&
            my2 >= bRefresh2.y && my2 < bRefresh2.y + bRefresh2.h)
            fetchGameList();

        if (mx2 >= bBack2.x && mx2 < bBack2.x + bBack2.w &&
            my2 >= bBack2.y && my2 < bBack2.y + bBack2.h)
        {
            out = ScreenResult::GO_MAIN_MENU;
            return true;
        }
    }
    return false;
}

void PlaceholderLobbyScreen::render()
{
    renderBackground();
    renderPanel();
    renderHeader();
    renderGameList();
    renderCreateSection();
    renderMapList();
    renderStatusBar();
}

void PlaceholderLobbyScreen::renderBackground()
{
    SDL_SetRenderDrawColor(renderer, C_BG.r, C_BG.g, C_BG.b, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 40, 30, 80, 30);
    for (int y = 0; y < windowH; y += 40)
        SDL_RenderDrawLine(renderer, 0, y, windowW, y);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void PlaceholderLobbyScreen::renderPanel()
{
    SDL_Rect shadow = {panel.x + 6, panel.y + 6, panel.w, panel.h};
    drawRect(shadow, {0, 0, 0, 120}, true);
    drawFilledRoundRect(panel, C_PANEL);
    drawBorderRoundRect(panel, C_PANEL_BORD);

    auto divider = [&](int y)
    {
        SDL_Rect d = {panel.x + 14, y, panel.w - 28, 1};
        drawRect(d, C_DIVIDER, true);
    };
    divider(panel.y + 78);
    divider(listArea.y + listArea.h + 6);
    divider(createRow.y + createRow.h + 6);
    divider(buttonArea.y - 8);
}

void PlaceholderLobbyScreen::renderHeader()
{
    SDL_Rect tr{};
    SDL_Texture *title = makeText("Argentum Online — Partidas", fontTitle, C_TITLE, tr);
    if (title)
    {
        tr.x = panel.x + (panel.w - tr.w) / 2;
        tr.y = panel.y + 14;
        drawTex(title, tr);
        SDL_DestroyTexture(title);
    }
    std::string sub = "Jugador: " + username +
                      "   |   F5 refrescar   |   Tab foco nombre   |   Esc volver";
    SDL_Rect sr{};
    SDL_Texture *subtex = makeText(sub, fontSmall, C_DIM, sr);
    if (subtex)
    {
        sr.x = panel.x + (panel.w - sr.w) / 2;
        sr.y = panel.y + 52;
        drawTex(subtex, sr);
        SDL_DestroyTexture(subtex);
    }
}

void PlaceholderLobbyScreen::renderGameList()
{
    {
        SDL_Rect r{};
        SDL_Texture *t = makeText("PARTIDAS DISPONIBLES", fontSmall, C_SUBTITLE, r);
        if (t)
        {
            r.x = listArea.x;
            r.y = listArea.y + 4;
            drawTex(t, r);
            SDL_DestroyTexture(t);
        }
    }

    auto col = [&](const char *lbl, int x)
    {
        SDL_Rect r{};
        SDL_Texture *t = makeText(lbl, fontSmall, C_SUBTITLE, r);
        if (t)
        {
            r.x = x;
            r.y = listArea.y + 4;
            drawTex(t, r);
            SDL_DestroyTexture(t);
        }
    };

    col("JUGADORES", listArea.x + listArea.w - 190);
    col("ESTADO", listArea.x + listArea.w - 90);

    const int startY = listArea.y + GAME_LIST_HEADER_GAP;

    if (games.empty())
    {
        SDL_Rect r{};
        SDL_Texture *t = makeText("No hay partidas. Crea una nueva abajo.", fontSmall, C_DIM, r);
        if (t)
        {
            r.x = listArea.x + (listArea.w - r.w) / 2;
            r.y = startY + 20;
            drawTex(t, r);
            SDL_DestroyTexture(t);
        }
        return;
    }

    const int visibleGames = visibleGamesCount();
    const int total = static_cast<int>(games.size());
    const int rowW = listArea.w - SCROLLBAR_W - SCROLLBAR_GAP;

    for (int i = 0; i < visibleGames; ++i)
    {
        int gameIdx = gamesScrollOffset + i;
        if (gameIdx >= total)
            break;

        const auto &g = games[gameIdx];
        bool sel = (gameIdx == selectedGame);
        bool hov = (gameIdx == hoveredGame);

        SDL_Rect row = {listArea.x, startY + i * (GAME_ROW_H + GAME_ROW_SP), rowW, GAME_ROW_H};
        SDL_Color bg = sel ? C_SEL_BG : hov        ? C_HOV_BG
                                    : (i % 2 == 0) ? C_ROW_EVEN
                                                   : C_ROW_ODD;

        drawFilledRoundRect(row, bg, 4);
        drawBorderRoundRect(row, sel ? C_SEL : (hov ? C_HOV : C_ROW_BORD), 4);

        if (sel)
        {
            SDL_Rect bar = {row.x, row.y + 6, 3, row.h - 12};
            drawRect(bar, C_SEL, true);
        }

        SDL_Color nc = sel ? C_SEL : C_TEXT;
        SDL_Rect nr{};
        SDL_Texture *nt = makeText(g.name, fontSmall, nc, nr);
        if (nt)
        {
            nr.x = row.x + 10;
            nr.y = row.y + (row.h - nr.h) / 2;
            drawTex(nt, nr);
            SDL_DestroyTexture(nt);
        }

        bool isFull = (g.players >= g.maxPlayers);
        std::string stText = isFull ? "LLENO" : "LIBRE";
        SDL_Color stColor = isFull ? C_FULL : C_AVAIL;

        SDL_Rect str{};
        SDL_Texture *st = makeText(stText, fontSmall, stColor, str);

        std::string pText = std::to_string(g.players) + " / " + std::to_string(g.maxPlayers);
        SDL_Rect pr{};
        SDL_Texture *pt = makeText(pText, fontSmall, C_DIM, pr);

        if (st && pt)
        {
            str.x = row.x + row.w - str.w - 10;
            str.y = row.y + (row.h - str.h) / 2;
            drawTex(st, str);

            SDL_Rect box = {str.x - 6, str.y - 2, str.w + 12, str.h + 4};
            drawBorderRoundRect(box, stColor, 3);

            pr.x = box.x - pr.w - 15;
            pr.y = row.y + (row.h - pr.h) / 2;
            drawTex(pt, pr);
        }
        if (st)
            SDL_DestroyTexture(st);
        if (pt)
            SDL_DestroyTexture(pt);
    }

    if (total > visibleGames)
    {
        const int scrollTrackH = visibleGames * (GAME_ROW_H + GAME_ROW_SP) - GAME_ROW_SP;
        const int scrollbarX = listArea.x + listArea.w - SCROLLBAR_W;
        SDL_Rect track = {scrollbarX, startY, SCROLLBAR_W, scrollTrackH};
        drawRect(track, {200, 200, 250, 25}, true);

        float visibleRatio = static_cast<float>(visibleGames) / total;
        int thumbH = std::max(16, static_cast<int>(scrollTrackH * visibleRatio));
        float scrollProgress = static_cast<float>(gamesScrollOffset) / (total - visibleGames);
        int thumbY = startY + static_cast<int>((scrollTrackH - thumbH) * scrollProgress);

        SDL_Rect thumb = {scrollbarX, thumbY, SCROLLBAR_W, thumbH};
        drawRect(thumb, C_SEL, true);
    }
}

void PlaceholderLobbyScreen::renderCreateSection()
{
    {
        SDL_Rect r{};
        SDL_Texture *t = makeText("NUEVA PARTIDA", fontSmall, C_SUBTITLE, r);
        if (t)
        {
            r.x = createRow.x;
            r.y = createRow.y + 2;
            drawTex(t, r);
            SDL_DestroyTexture(t);
        }
    }

    const int createBtnW = 110;
    const int inputH = 36;
    const int inputY = createRow.y + CREATE_INPUT_Y_OFFSET;

    SDL_Rect inputBox = {createRow.x, inputY, createRow.w - createBtnW - 8, inputH};
    SDL_Color inputBg = typingNewGame ? C_INPUT_ACT : C_INPUT_BG;
    drawFilledRoundRect(inputBox, inputBg, 4);
    drawBorderRoundRect(inputBox, typingNewGame ? C_SEL : C_ROW_BORD, 4);

    std::string display = newGameName;
    if (typingNewGame && (SDL_GetTicks() / 500) % 2 == 0)
        display += "|";
    if (display.empty() && !typingNewGame)
        display = "Nombre de la partida...";

    SDL_Color textColor = (!newGameName.empty() || typingNewGame) ? C_TEXT : C_DIM;
    SDL_Rect txtr{};
    SDL_Texture *txt = makeText(display, fontSmall, textColor, txtr);
    if (txt)
    {
        txtr.x = inputBox.x + 10;
        txtr.y = inputBox.y + (inputBox.h - txtr.h) / 2;
        drawTex(txt, txtr);
        SDL_DestroyTexture(txt);
    }

    SDL_Rect bCreate = {createRow.x + createRow.w - createBtnW, inputY, createBtnW, inputH};
    drawButton(bCreate, "Crear", C_BTN_CRE, C_AVAIL, hoverCreate);
}

void PlaceholderLobbyScreen::renderMapList()
{
    {
        SDL_Rect r{};
        SDL_Texture *t = makeText("SELECCIONAR MAPA", fontSmall, C_SUBTITLE, r);
        if (t)
        {
            r.x = mapListArea.x;
            r.y = mapListArea.y + 4;
            drawTex(t, r);
            SDL_DestroyTexture(t);
        }
    }

    const int startY = mapListArea.y + MAP_LIST_HEADER_GAP;

    if (maps.empty())
    {
        SDL_Rect r{};
        SDL_Texture *t = makeText("No hay mapas en assets/sprites/MapAssets/worlds/",
                                  fontSmall, C_DIM, r);
        if (t)
        {
            r.x = mapListArea.x;
            r.y = startY;
            drawTex(t, r);
            SDL_DestroyTexture(t);
        }
        return;
    }

    const int textPad = 14;
    const int visibleMaps = visibleMapsCount();
    const int total = static_cast<int>(maps.size());
    const int rowW = mapListArea.w - SCROLLBAR_W - SCROLLBAR_GAP;

    for (int i = 0; i < visibleMaps; ++i)
    {
        int mapIdx = mapScrollOffset + i;
        if (mapIdx >= total)
            break;

        bool sel = (mapIdx == selectedMap);
        bool hov = (mapIdx == hoveredMap);

        SDL_Rect row = {mapListArea.x, startY + i * (MAP_ROW_H + MAP_ROW_SP), rowW, MAP_ROW_H};

        SDL_Color bg = sel ? C_MAP_SEL_BG : hov        ? C_HOV_BG
                                        : (i % 2 == 0) ? C_ROW_EVEN
                                                       : C_ROW_ODD;
        drawFilledRoundRect(row, bg, 4);
        drawBorderRoundRect(row, sel ? C_MAP_SEL : (hov ? C_HOV : C_ROW_BORD), 4);

        if (sel)
        {
            SDL_Rect bar = {row.x, row.y + 6, 3, row.h - 12};
            drawRect(bar, C_MAP_SEL, true);
        }

        SDL_Color nc = sel ? C_MAP_SEL : (hov ? C_HOV : C_TEXT);
        SDL_Rect nr{};
        SDL_Texture *nt = makeText(maps[mapIdx].name, fontMedium, nc, nr);
        if (nt)
        {
            nr.x = row.x + textPad;
            nr.y = row.y + (row.h - nr.h) / 2;
            drawTex(nt, nr);
            SDL_DestroyTexture(nt);
        }

        std::string dims = std::to_string(maps[mapIdx].width) + " × " +
                           std::to_string(maps[mapIdx].height) + " tiles";
        SDL_Rect dr{};
        SDL_Texture *dt = makeText(dims, fontSmall, C_DIM, dr);
        if (dt)
        {
            dr.x = row.x + row.w - dr.w - textPad;
            dr.y = row.y + (row.h - dr.h) / 2;
            drawTex(dt, dr);
            SDL_DestroyTexture(dt);
        }
    }

    if (total > visibleMaps)
    {
        const int scrollTrackH = visibleMaps * (MAP_ROW_H + MAP_ROW_SP) - MAP_ROW_SP;
        const int scrollbarX = mapListArea.x + mapListArea.w - SCROLLBAR_W;
        SDL_Rect track = {scrollbarX, startY, SCROLLBAR_W, scrollTrackH};
        drawRect(track, {200, 200, 250, 25}, true);

        float visibleRatio = static_cast<float>(visibleMaps) / total;
        int thumbH = std::max(16, static_cast<int>(scrollTrackH * visibleRatio));
        float scrollProgress = static_cast<float>(mapScrollOffset) / (total - visibleMaps);
        int thumbY = startY + static_cast<int>((scrollTrackH - thumbH) * scrollProgress);

        SDL_Rect thumb = {scrollbarX, thumbY, SCROLLBAR_W, thumbH};
        drawRect(thumb, C_MAP_SEL, true);
    }
}

void PlaceholderLobbyScreen::renderStatusBar()
{
    const int bw = (buttonArea.w - 20) / 3;
    SDL_Rect bJoin = {buttonArea.x, buttonArea.y, bw, buttonArea.h};
    SDL_Rect bRefresh = {buttonArea.x + bw + 10, buttonArea.y, bw, buttonArea.h};
    SDL_Rect bBack = {buttonArea.x + (bw + 10) * 2, buttonArea.y, bw, buttonArea.h};

    drawButton(bJoin, "Unirse", C_BTN_JOIN, {60, 120, 200, 255}, hoverJoin);
    drawButton(bRefresh, "Refrescar", C_BTN_REF, {140, 140, 60, 255}, hoverRefresh);
    drawButton(bBack, "Volver", C_BTN_BACK, {180, 60, 60, 255}, hoverBack);

    const int msgY = panel.y + panel.h + 8;
    if (!errorMsg.empty())
    {
        SDL_Rect r{};
        SDL_Texture *t = makeText(errorMsg, fontSmall, C_ERROR, r);
        if (t)
        {
            r.x = windowW / 2 - r.w / 2;
            r.y = msgY;
            drawTex(t, r);
            SDL_DestroyTexture(t);
        }
    }
    else if (!statusMsg.empty())
    {
        SDL_Rect r{};
        SDL_Texture *t = makeText(statusMsg, fontSmall, C_STATUS, r);
        if (t)
        {
            r.x = windowW / 2 - r.w / 2;
            r.y = msgY;
            drawTex(t, r);
            SDL_DestroyTexture(t);
        }
    }
}

void PlaceholderLobbyScreen::drawFilledRoundRect(const SDL_Rect &r, SDL_Color color, int radius)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect center = {r.x + radius, r.y, r.w - radius * 2, r.h};
    SDL_RenderFillRect(renderer, &center);
    SDL_Rect left = {r.x, r.y + radius, radius, r.h - radius * 2};
    SDL_Rect right = {r.x + r.w - radius, r.y + radius, radius, r.h - radius * 2};
    SDL_RenderFillRect(renderer, &left);
    SDL_RenderFillRect(renderer, &right);
    for (int dy = 0; dy < radius; dy++)
    {
        int dx = static_cast<int>(std::sqrt(static_cast<float>(radius * radius - dy * dy)));
        SDL_RenderDrawLine(renderer, r.x + radius - dx, r.y + dy, r.x + radius, r.y + dy);
        SDL_RenderDrawLine(renderer, r.x + r.w - radius, r.y + dy, r.x + r.w - radius + dx - 1, r.y + dy);
        SDL_RenderDrawLine(renderer, r.x + radius - dx, r.y + r.h - 1 - dy, r.x + radius, r.y + r.h - 1 - dy);
        SDL_RenderDrawLine(renderer, r.x + r.w - radius, r.y + r.h - 1 - dy, r.x + r.w - radius + dx - 1, r.y + r.h - 1 - dy);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void PlaceholderLobbyScreen::drawBorderRoundRect(const SDL_Rect &r, SDL_Color color, int radius)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(renderer, r.x + radius, r.y, r.x + r.w - radius, r.y);
    SDL_RenderDrawLine(renderer, r.x + radius, r.y + r.h - 1, r.x + r.w - radius, r.y + r.h - 1);
    SDL_RenderDrawLine(renderer, r.x, r.y + radius, r.x, r.y + r.h - radius);
    SDL_RenderDrawLine(renderer, r.x + r.w - 1, r.y + radius, r.x + r.w - 1, r.y + r.h - radius);
    for (int dy = 0; dy < radius; dy++)
    {
        int dx = static_cast<int>(std::sqrt(static_cast<float>(radius * radius - dy * dy)));
        SDL_RenderDrawPoint(renderer, r.x + radius - dx, r.y + dy);
        SDL_RenderDrawPoint(renderer, r.x + r.w - radius + dx - 1, r.y + dy);
        SDL_RenderDrawPoint(renderer, r.x + radius - dx, r.y + r.h - 1 - dy);
        SDL_RenderDrawPoint(renderer, r.x + r.w - radius + dx - 1, r.y + r.h - 1 - dy);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void PlaceholderLobbyScreen::drawButton(const SDL_Rect &r, const char *label,
                                        SDL_Color bg, SDL_Color border, bool hovered)
{
    SDL_Color activeBg = hovered
                             ? SDL_Color{static_cast<Uint8>(std::min(bg.r + 30, 255)),
                                         static_cast<Uint8>(std::min(bg.g + 30, 255)),
                                         static_cast<Uint8>(std::min(bg.b + 30, 255)), bg.a}
                             : bg;
    drawFilledRoundRect(r, activeBg, 5);
    drawBorderRoundRect(r, hovered ? C_HOV : border, 5);
    SDL_Rect tr{};
    SDL_Color labelColor = hovered ? C_HOV : C_TEXT;
    SDL_Texture *t = makeText(label, fontSmall, labelColor, tr);
    if (t)
    {
        tr.x = r.x + (r.w - tr.w) / 2;
        tr.y = r.y + (r.h - tr.h) / 2;
        drawTex(t, tr);
        SDL_DestroyTexture(t);
    }
}

SDL_Texture *PlaceholderLobbyScreen::makeText(const std::string &text, TTF_Font *font,
                                              SDL_Color color, SDL_Rect &out)
{
    if (text.empty())
        return nullptr;
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!s)
        return nullptr;
    out.w = s->w;
    out.h = s->h;
    SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
    SDL_FreeSurface(s);
    return t;
}

void PlaceholderLobbyScreen::drawTex(SDL_Texture *tex, const SDL_Rect &dst)
{
    SDL_RenderCopy(renderer, tex, nullptr, &dst);
}

void PlaceholderLobbyScreen::drawRect(const SDL_Rect &r, SDL_Color color, bool fill)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    if (fill)
        SDL_RenderFillRect(renderer, &r);
    else
        SDL_RenderDrawRect(renderer, &r);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

const PlayerDto &PlaceholderLobbyScreen::getJoinedPlayerDto() const
{
    return joinedPlayerDto;
}
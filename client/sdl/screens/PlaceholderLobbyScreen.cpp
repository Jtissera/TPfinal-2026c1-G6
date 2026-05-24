#include "PlaceholderLobbyScreen.h"

#include <stdexcept>
#include "../../../common/network/messages/client/lobby/listGamesMessage.h"
#include "../../../common/network/messages/client/lobby/createGameMessage.h"
#include "../../../common/network/messages/client/lobby/joinGameMessage.h"
#include "../../../common/network/messages/server/lobby/gameCreatedMessage.h"
#include "../../../common/network/messages/server/lobby/joinOkMessage.h"
#include "../../../common/network/messages/server/error/errorMessage.h"
#include "../../../common/network/protocol/serverOpCode.h"

PlaceholderLobbyScreen::PlaceholderLobbyScreen(SDL_Renderer* renderer, int windowW, int windowH,
                                                const std::string& fontPath,
                                                Protocol& protocol,
                                                const std::string& username)
    : renderer(renderer), windowW(windowW), windowH(windowH),
      protocol(protocol), username(username)
{
    if (TTF_WasInit() == 0 && TTF_Init() == -1)
        throw std::runtime_error(std::string("TTF_Init: ") + TTF_GetError());

    fontMedium = TTF_OpenFont(fontPath.c_str(), 22);
    fontSmall  = TTF_OpenFont(fontPath.c_str(), 16);
    if (!fontMedium || !fontSmall)
        throw std::runtime_error(std::string("TTF_OpenFont: ") + TTF_GetError());

    SDL_StartTextInput();
    fetchGameList();
}

PlaceholderLobbyScreen::~PlaceholderLobbyScreen() {
    SDL_StopTextInput();
    if (fontMedium) { TTF_CloseFont(fontMedium); fontMedium = nullptr; }
    if (fontSmall)  { TTF_CloseFont(fontSmall);  fontSmall  = nullptr; }
}

// ─────────────────────────────────────────────────────────────────────────────
// Comunicación con el servidor
// ─────────────────────────────────────────────────────────────────────────────

void PlaceholderLobbyScreen::fetchGameList() {
    try {
        protocol.send(ListGamesMessage());
        auto response = protocol.receive();
        if (response->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_GAME_LIST)) {
            const auto& msg = static_cast<const GameListMessage&>(*response);
            games.clear();
            for (const auto& g : msg.getGames())
                games.push_back({g.gameId, g.gameName, g.playerCount, g.maxPlayers});
            if (selectedGame >= static_cast<int>(games.size()))
                selectedGame = games.empty() ? -1 : 0;
            errorMsg.clear();
        }
    } catch (const std::exception& e) {
        errorMsg = std::string("Error al listar: ") + e.what();
    }
}

void PlaceholderLobbyScreen::tryCreateGame() {
    if (newGameName.empty()) {
        errorMsg = "Ingresa un nombre para la partida.";
        return;
    }
    try {
        protocol.send(CreateGameMessage(newGameName, 4));
        auto response = protocol.receive();
        if (response->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_GAME_CREATED)) {
            const auto& created = static_cast<const GameCreatedMessage&>(*response);
            protocol.send(JoinGameMessage(created.getGameId()));
            auto joinResponse = protocol.receive();
            if (joinResponse->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_JOIN_OK)) {
                // Señal al llamador de que entramos al juego — lo manejamos
                // marcando un flag; run() lo detecta y devuelve GO_LOBBY.
                _readyToPlay = true;
            } else if (joinResponse->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_ERROR)) {
                const auto& err = static_cast<const ErrorMessage&>(*joinResponse);
                errorMsg = err.getReason();
            }
        } else if (response->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_ERROR)) {
            const auto& err = static_cast<const ErrorMessage&>(*response);
            errorMsg = err.getReason();
        }
    } catch (const std::exception& e) {
        errorMsg = std::string("Error: ") + e.what();
    }
}

void PlaceholderLobbyScreen::tryJoinSelected() {
    if (selectedGame < 0 || selectedGame >= static_cast<int>(games.size())) {
        errorMsg = "Selecciona una partida primero.";
        return;
    }
    try {
        protocol.send(JoinGameMessage(games[selectedGame].id));
        auto response = protocol.receive();
        if (response->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_JOIN_OK)) {
            _readyToPlay = true;
        } else if (response->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_ERROR)) {
            const auto& err = static_cast<const ErrorMessage&>(*response);
            errorMsg = err.getReason();
        }
    } catch (const std::exception& e) {
        errorMsg = std::string("Error: ") + e.what();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Loop principal
// ─────────────────────────────────────────────────────────────────────────────

ScreenResult PlaceholderLobbyScreen::run() {
    while (true) {
        if (_readyToPlay) return ScreenResult::GO_LOBBY;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return ScreenResult::QUIT;

            ScreenResult res;
            if (handleEvent(e, res)) return res;
        }
        render();
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Input
// ─────────────────────────────────────────────────────────────────────────────

bool PlaceholderLobbyScreen::handleEvent(const SDL_Event& e, ScreenResult& out) {
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
            case SDLK_ESCAPE:
                out = ScreenResult::GO_MAIN_MENU;
                return true;
            case SDLK_BACKSPACE:
                if (typingNewGame && !newGameName.empty())
                    newGameName.pop_back();
                break;
            case SDLK_RETURN: case SDLK_KP_ENTER:
                if (typingNewGame) tryCreateGame();
                else if (selectedGame >= 0) tryJoinSelected();
                break;
            case SDLK_UP:
                if (!games.empty())
                    selectedGame = (selectedGame <= 0)
                        ? static_cast<int>(games.size()) - 1
                        : selectedGame - 1;
                typingNewGame = false;
                break;
            case SDLK_DOWN:
                if (!games.empty())
                    selectedGame = (selectedGame + 1) % static_cast<int>(games.size());
                typingNewGame = false;
                break;
            case SDLK_F5:
                fetchGameList();
                break;
            default: break;
        }
    }

    if (e.type == SDL_TEXTINPUT && typingNewGame) {
        if (newGameName.size() < 24)
            newGameName += e.text.text;
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;
        const int cx = windowW / 2;

        // Lista de partidas
        const int listY = 140, rowH = 36, rowSpacing = 6;
        for (int i = 0; i < static_cast<int>(games.size()); ++i) {
            SDL_Rect row = {cx - 240, listY + i * (rowH + rowSpacing), 480, rowH};
            if (mx >= row.x && mx <= row.x + row.w &&
                my >= row.y && my <= row.y + row.h) {
                selectedGame  = i;
                typingNewGame = false;
            }
        }

        // Botones
        int btnY = windowH - 130;
        SDL_Rect btnJoin    = {cx - 250, btnY,      140, 38};
        SDL_Rect btnRefresh = {cx - 80,  btnY,      140, 38};
        SDL_Rect btnBack    = {cx + 90,  btnY,      140, 38};
        SDL_Rect inputBox   = {cx - 240, btnY + 54, 340, 36};
        SDL_Rect btnCreate  = {cx + 120, btnY + 54, 100, 36};

        if (mx >= btnJoin.x && mx <= btnJoin.x + btnJoin.w &&
            my >= btnJoin.y && my <= btnJoin.y + btnJoin.h)
            tryJoinSelected();

        if (mx >= btnRefresh.x && mx <= btnRefresh.x + btnRefresh.w &&
            my >= btnRefresh.y && my <= btnRefresh.y + btnRefresh.h)
            fetchGameList();

        if (mx >= btnBack.x && mx <= btnBack.x + btnBack.w &&
            my >= btnBack.y && my <= btnBack.y + btnBack.h) {
            out = ScreenResult::GO_MAIN_MENU;
            return true;
        }

        if (mx >= inputBox.x && mx <= inputBox.x + inputBox.w &&
            my >= inputBox.y && my <= inputBox.y + inputBox.h)
            typingNewGame = true;

        if (mx >= btnCreate.x && mx <= btnCreate.x + btnCreate.w &&
            my >= btnCreate.y && my <= btnCreate.y + btnCreate.h)
            tryCreateGame();
    }

    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Renderizado
// ─────────────────────────────────────────────────────────────────────────────

void PlaceholderLobbyScreen::render() {
    SDL_SetRenderDrawColor(renderer, C_BG.r, C_BG.g, C_BG.b, 255);
    SDL_RenderClear(renderer);
    renderTitle();
    renderGameList();
    renderInput();
    renderButtons();
    renderError();
}

void PlaceholderLobbyScreen::renderTitle() {
    SDL_Rect r{};
    SDL_Texture* t = makeText("Lobby — " + username, fontMedium, C_TITLE, r);
    if (!t) return;
    r.x = windowW / 2 - r.w / 2; r.y = 30;
    drawTex(t, r); SDL_DestroyTexture(t);

    SDL_Rect sub{};
    SDL_Texture* s = makeText("F5 para refrescar  |  Esc para volver", fontSmall, C_DIM, sub);
    if (s) { sub.x = windowW/2 - sub.w/2; sub.y = 68; drawTex(s, sub); SDL_DestroyTexture(s); }
}

void PlaceholderLobbyScreen::renderGameList() {
    const int cx = windowW / 2;
    const int listY = 110, rowH = 36, rowSpacing = 6;

    if (games.empty()) {
        SDL_Rect r{};
        SDL_Texture* t = makeText("No hay partidas disponibles.", fontSmall, C_DIM, r);
        if (t) { r.x = cx - r.w/2; r.y = listY + 10; drawTex(t, r); SDL_DestroyTexture(t); }
        return;
    }

    for (int i = 0; i < static_cast<int>(games.size()); ++i) {
        SDL_Rect row = {cx - 240, listY + i * (rowH + rowSpacing), 480, rowH};
        bool sel = (i == selectedGame);

        if (sel) {
            drawRect(row, C_SEL_BG, true);
            SDL_SetRenderDrawColor(renderer, C_SEL.r, C_SEL.g, C_SEL.b, 200);
            SDL_RenderDrawRect(renderer, &row);
        } else {
            SDL_Color bg = {25, 22, 45, 200};
            drawRect(row, bg, true);
            SDL_SetRenderDrawColor(renderer, 60, 55, 90, 255);
            SDL_RenderDrawRect(renderer, &row);
        }

        std::string label = games[i].name + "  [" +
                            std::to_string(games[i].players) + "/" +
                            std::to_string(games[i].maxPlayers) + "]";
        SDL_Rect txt{};
        SDL_Texture* t = makeText(label, fontSmall, sel ? C_SEL : C_TEXT, txt);
        if (t) {
            txt.x = row.x + 12;
            txt.y = row.y + (row.h - txt.h) / 2;
            drawTex(t, txt); SDL_DestroyTexture(t);
        }
    }
}

void PlaceholderLobbyScreen::renderInput() {
    const int cx = windowW / 2;
    const int btnY = windowH - 130;

    // Label
    SDL_Rect lbl{};
    SDL_Texture* lt = makeText("Nueva partida:", fontSmall, C_DIM, lbl);
    if (lt) { lbl.x = cx - 240; lbl.y = btnY + 36; drawTex(lt, lbl); SDL_DestroyTexture(lt); }

    // Caja de texto
    SDL_Rect box = {cx - 240, btnY + 54, 340, 36};
    SDL_Color boxBg = typingNewGame ? C_SEL_BG : SDL_Color{25, 22, 45, 200};
    drawRect(box, boxBg, true);
    SDL_Color border = typingNewGame ? C_SEL : SDL_Color{60, 55, 90, 255};
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, 255);
    SDL_RenderDrawRect(renderer, &box);

    std::string display = newGameName;
    if (typingNewGame && (SDL_GetTicks() / 500) % 2 == 0) display += "|";
    if (!display.empty()) {
        SDL_Rect txt{};
        SDL_Texture* t = makeText(display, fontSmall, C_TEXT, txt);
        if (t) { txt.x = box.x + 8; txt.y = box.y + (box.h - txt.h)/2; drawTex(t, txt); SDL_DestroyTexture(t); }
    }

    // Botón Crear
    SDL_Rect btn = {cx + 120, btnY + 54, 100, 36};
    SDL_Color btnBg = {40, 120, 60, 200};
    drawRect(btn, btnBg, true);
    SDL_SetRenderDrawColor(renderer, 60, 180, 90, 255);
    SDL_RenderDrawRect(renderer, &btn);
    SDL_Rect bt{};
    SDL_Texture* btt = makeText("Crear", fontSmall, C_TEXT, bt);
    if (btt) { bt.x = btn.x + (btn.w - bt.w)/2; bt.y = btn.y + (btn.h - bt.h)/2; drawTex(btt, bt); SDL_DestroyTexture(btt); }
}

void PlaceholderLobbyScreen::renderButtons() {
    const int cx = windowW / 2;
    const int btnY = windowH - 130;

    auto makeBtn = [&](const SDL_Rect& r, const char* label, SDL_Color bg, SDL_Color border) {
        drawRect(r, bg, true);
        SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, 255);
        SDL_RenderDrawRect(renderer, &r);
        SDL_Rect t{};
        SDL_Texture* tex = makeText(label, fontSmall, C_TEXT, t);
        if (tex) { t.x = r.x + (r.w - t.w)/2; t.y = r.y + (r.h - t.h)/2; drawTex(tex, t); SDL_DestroyTexture(tex); }
    };

    makeBtn({cx - 250, btnY, 140, 38}, "Unirse",    {40, 80, 140, 200},  {60, 120, 200, 255});
    makeBtn({cx - 80,  btnY, 140, 38}, "Refrescar", {60, 60, 30,  200},  {140, 140, 60, 255});
    makeBtn({cx + 90,  btnY, 140, 38}, "Volver",    {100, 30, 30, 200},  {180, 60,  60, 255});
}

void PlaceholderLobbyScreen::renderError() {
    if (errorMsg.empty()) return;
    SDL_Rect r{};
    SDL_Texture* t = makeText(errorMsg, fontSmall, C_ERROR, r);
    if (!t) return;
    r.x = windowW/2 - r.w/2; r.y = windowH - 30;
    drawTex(t, r); SDL_DestroyTexture(t);
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

SDL_Texture* PlaceholderLobbyScreen::makeText(const std::string& text, TTF_Font* font,
                                               SDL_Color color, SDL_Rect& out) {
    SDL_Surface* s = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!s) return nullptr;
    out.w = s->w; out.h = s->h;
    SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
    SDL_FreeSurface(s);
    return t;
}

void PlaceholderLobbyScreen::drawTex(SDL_Texture* tex, const SDL_Rect& dst) {
    SDL_RenderCopy(renderer, tex, nullptr, &dst);
}

void PlaceholderLobbyScreen::drawRect(const SDL_Rect& r, SDL_Color color, bool fill) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    if (fill) SDL_RenderFillRect(renderer, &r);
    else       SDL_RenderDrawRect(renderer, &r);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

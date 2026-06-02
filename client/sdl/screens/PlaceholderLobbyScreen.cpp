#include "PlaceholderLobbyScreen.h"

#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <iostream>

#include "../../../common/network/messages/client/lobby/listGamesMessage.h"
#include "../../../common/network/messages/client/lobby/createGameMessage.h"
#include "../../../common/network/messages/client/lobby/joinGameMessage.h"
#include "../../../common/network/messages/server/lobby/gameCreatedMessage.h"
#include "../../../common/network/messages/server/lobby/joinOkMessage.h"
#include "../../../common/network/messages/server/error/errorMessage.h"
#include "../../../common/network/protocol/serverOpCode.h"

// Constructor / Destructor
PlaceholderLobbyScreen::PlaceholderLobbyScreen(SDL_Renderer* renderer,
                                                int windowW, int windowH,
                                                const std::string& fontPath,
                                                Protocol& protocol,
                                                const std::string& username)
    : renderer(renderer), windowW(windowW), windowH(windowH),
      protocol(protocol), username(username)
{
    if (TTF_WasInit() == 0 && TTF_Init() == -1)
        throw std::runtime_error(std::string("TTF_Init: ") + TTF_GetError());

    fontTitle  = TTF_OpenFont(fontPath.c_str(), 32);
    fontMedium = TTF_OpenFont(fontPath.c_str(), 20);
    fontSmall  = TTF_OpenFont(fontPath.c_str(), 15);

    if (!fontTitle || !fontMedium || !fontSmall)
        throw std::runtime_error(std::string("TTF_OpenFont: ") + TTF_GetError());

    computeLayout();
    SDL_StartTextInput();
    fetchGameList();
}

PlaceholderLobbyScreen::~PlaceholderLobbyScreen() {
    SDL_StopTextInput();
    if (fontTitle)  { TTF_CloseFont(fontTitle);  fontTitle  = nullptr; }
    if (fontMedium) { TTF_CloseFont(fontMedium); fontMedium = nullptr; }
    if (fontSmall)  { TTF_CloseFont(fontSmall);  fontSmall  = nullptr; }
}

void PlaceholderLobbyScreen::computeLayout() {
    // Panel central: 600 px de ancho, casi toda la altura
    const int panelW = 620;
    const int panelH = windowH - 80;
    panel = {(windowW - panelW) / 2, 40, panelW, panelH};

    // Seccion lista de partidas
    const int headerH   = 80;
    const int padH      = 12;
    const int sectionPad = 14;

    listArea = {
        panel.x + sectionPad,
        panel.y + headerH + padH,
        panelW - sectionPad * 2,
        static_cast<int>(panelH * 0.50f)
    };

    // Seccion crear partida
    const int createH = 80;
    createArea = {
        panel.x + sectionPad,
        listArea.y + listArea.h + 14,
        panelW - sectionPad * 2,
        createH
    };

    // Zona de botones principales
    const int btnH = 40;
    buttonArea = {
        panel.x + sectionPad,
        panel.y + panelH - btnH - sectionPad,
        panelW - sectionPad * 2,
        btnH
    };
}

// Comunicación con el servidor
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
            statusMsg = "Lista actualizada.";
        }
    } catch (const std::exception& e) {
        errorMsg = std::string("Error al listar: ") + e.what();
        statusMsg.clear();
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
                const auto& joinOk = static_cast<const JoinOkMessage&>(*joinResponse);

                joinedPlayerDto = joinOk.getPlayerDto();

                std::cout << "[LOBBY DTO CREATE] raza='"
                          << joinedPlayerDto.raza
                          << "' clase='"
                          << joinedPlayerDto.clase
                          << "'"
                          << std::endl;

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
    if (games[selectedGame].players >= games[selectedGame].maxPlayers) {
        errorMsg = "La partida está llena.";
        return;
    }
    try {
        protocol.send(JoinGameMessage(games[selectedGame].id));
        auto response = protocol.receive();
        if (response->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_JOIN_OK)) {
            const auto& joinOk = static_cast<const JoinOkMessage&>(*response);
            joinedPlayerDto = joinOk.getPlayerDto();
            _readyToPlay = true;
        } else if (response->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_ERROR)) {
            const auto& err = static_cast<const ErrorMessage&>(*response);
            errorMsg = err.getReason();
        }
    } catch (const std::exception& e) {
        errorMsg = std::string("Error: ") + e.what();
    }
}

// Loop principal
ScreenResult PlaceholderLobbyScreen::run() {
    while (true) {
        if (_readyToPlay) return ScreenResult::GO_LOBBY;

        // Actualizar hover del mouse cada frame
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        // Hover sobre filas
        hoveredGame = -1;
        const int rowH = 38, rowSpacing = 5;
        for (int i = 0; i < static_cast<int>(games.size()); ++i) {
            SDL_Rect row = {listArea.x, listArea.y + 28 + i * (rowH + rowSpacing),
                            listArea.w, rowH};
            if (mx >= row.x && mx <= row.x + row.w &&
                my >= row.y && my <= row.y + row.h)
                hoveredGame = i;
        }

        // Hover sobre botones
        const int bw = (buttonArea.w - 20) / 3;
        SDL_Rect bJoin    = {buttonArea.x,           buttonArea.y, bw, buttonArea.h};
        SDL_Rect bRefresh = {buttonArea.x + bw + 10, buttonArea.y, bw, buttonArea.h};
        SDL_Rect bBack    = {buttonArea.x + (bw+10)*2, buttonArea.y, bw, buttonArea.h};
        SDL_Rect bCreate  = {createArea.x + createArea.w - 110,
                             createArea.y + 34, 100, 32};

        auto inRect = [&](SDL_Rect r) {
            return mx >= r.x && mx <= r.x+r.w && my >= r.y && my <= r.y+r.h;
        };
        hoverJoin    = inRect(bJoin);
        hoverRefresh = inRect(bRefresh);
        hoverBack    = inRect(bBack);
        hoverCreate  = inRect(bCreate);

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

// Input
bool PlaceholderLobbyScreen::handleEvent(const SDL_Event& e, ScreenResult& out) {
    if (e.type == SDL_KEYDOWN) {
        errorMsg.clear();
        statusMsg.clear();
        switch (e.key.keysym.sym) {
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
            case SDLK_RETURN: case SDLK_KP_ENTER:
                if (typingNewGame) tryCreateGame();
                else if (selectedGame >= 0) tryJoinSelected();
                break;
            case SDLK_UP: case SDLK_w:
                if (!games.empty()) {
                    selectedGame = (selectedGame <= 0)
                        ? static_cast<int>(games.size()) - 1
                        : selectedGame - 1;
                }
                typingNewGame = false;
                break;
            case SDLK_DOWN: case SDLK_s:
                if (!games.empty()) {
                    selectedGame = (selectedGame + 1) % static_cast<int>(games.size());
                }
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
        errorMsg.clear();

        // Click en fila de partidas
        const int rowH = 38, rowSpacing = 5;
        for (int i = 0; i < static_cast<int>(games.size()); ++i) {
            SDL_Rect row = {listArea.x, listArea.y + 28 + i * (rowH + rowSpacing),
                            listArea.w, rowH};
            if (mx >= row.x && mx <= row.x + row.w &&
                my >= row.y && my <= row.y + row.h) {
                if (selectedGame == i) {
                    tryJoinSelected();
                } else {
                    selectedGame  = i;
                    typingNewGame = false;
                }
            }
        }

        // Click en campo de texto
        SDL_Rect inputBox = {createArea.x, createArea.y + 34,
                             createArea.w - 120, 32};
        if (mx >= inputBox.x && mx <= inputBox.x + inputBox.w &&
            my >= inputBox.y && my <= inputBox.y + inputBox.h)
            typingNewGame = true;

        // Click en boton Crear
        SDL_Rect bCreate = {createArea.x + createArea.w - 110,
                            createArea.y + 34, 100, 32};
        if (mx >= bCreate.x && mx <= bCreate.x + bCreate.w &&
            my >= bCreate.y && my <= bCreate.y + bCreate.h)
            tryCreateGame();

        // Botones principales
        const int bw = (buttonArea.w - 20) / 3;
        SDL_Rect bJoin    = {buttonArea.x,            buttonArea.y, bw, buttonArea.h};
        SDL_Rect bRefresh = {buttonArea.x + bw + 10,  buttonArea.y, bw, buttonArea.h};
        SDL_Rect bBack    = {buttonArea.x + (bw+10)*2, buttonArea.y, bw, buttonArea.h};

        if (mx >= bJoin.x && mx <= bJoin.x+bJoin.w &&
            my >= bJoin.y && my <= bJoin.y+bJoin.h)
            tryJoinSelected();

        if (mx >= bRefresh.x && mx <= bRefresh.x+bRefresh.w &&
            my >= bRefresh.y && my <= bRefresh.y+bRefresh.h)
            fetchGameList();

        if (mx >= bBack.x && mx <= bBack.x+bBack.w &&
            my >= bBack.y && my <= bBack.y+bBack.h) {
            out = ScreenResult::GO_MAIN_MENU;
            return true;
        }
    }

    return false;
}

// Renderizado
void PlaceholderLobbyScreen::render() {
    renderBackground();
    renderPanel();
    renderHeader();
    renderGameList();
    renderCreateSection();
    renderStatusBar();
}

void PlaceholderLobbyScreen::renderBackground() {
    SDL_SetRenderDrawColor(renderer, C_BG.r, C_BG.g, C_BG.b, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 40, 30, 80, 30);
    for (int y = 0; y < windowH; y += 40)
        SDL_RenderDrawLine(renderer, 0, y, windowW, y);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void PlaceholderLobbyScreen::renderPanel() {
    SDL_Rect shadow = {panel.x + 6, panel.y + 6, panel.w, panel.h};
    drawRect(shadow, {0, 0, 0, 120}, true);

    // Panel principal
    drawFilledRoundRect(panel, C_PANEL);
    drawBorderRoundRect(panel, C_PANEL_BORD);

    // Separador debajo del header
    SDL_Rect divider = {panel.x + 14, panel.y + 72, panel.w - 28, 1};
    drawRect(divider, C_DIVIDER, true);

    // Separador encima de botones
    SDL_Rect divider2 = {panel.x + 14, buttonArea.y - 10, panel.w - 28, 1};
    drawRect(divider2, C_DIVIDER, true);
}

void PlaceholderLobbyScreen::renderHeader() {
    SDL_Rect tr{};
    SDL_Texture* title = makeText("Argentum Online — Partidas", fontTitle, C_TITLE, tr);
    if (title) {
        tr.x = panel.x + (panel.w - tr.w) / 2;
        tr.y = panel.y + 18;
        drawTex(title, tr);
        SDL_DestroyTexture(title);
    }

    std::string sub = "Jugador: " + username + "   |   F5 refrescar   |   Tab cambiar foco   |   Esc volver";
    SDL_Rect sr{};
    SDL_Texture* subtex = makeText(sub, fontSmall, C_DIM, sr);
    if (subtex) {
        sr.x = panel.x + (panel.w - sr.w) / 2;
        sr.y = panel.y + 56;
        drawTex(subtex, sr);
        SDL_DestroyTexture(subtex);
    }
}

void PlaceholderLobbyScreen::renderGameList() {
    SDL_Rect hdr{};
    SDL_Texture* hdrTex = makeText("PARTIDAS DISPONIBLES", fontSmall, C_SUBTITLE, hdr);
    if (hdrTex) {
        hdr.x = listArea.x;
        hdr.y = listArea.y + 4;
        drawTex(hdrTex, hdr);
        SDL_DestroyTexture(hdrTex);
    }

    // Columnas: Nombre | Jugadores | Estado
    auto col = [&](const char* lbl, int x, SDL_Color c) {
        SDL_Rect r{};
        SDL_Texture* t = makeText(lbl, fontSmall, c, r);
        if (t) { r.x = x; r.y = listArea.y + 4; drawTex(t, r); SDL_DestroyTexture(t); }
    };
    col("JUGADORES", listArea.x + listArea.w - 190, C_SUBTITLE);
    col("ESTADO",    listArea.x + listArea.w - 90,  C_SUBTITLE);

    const int rowH = 38, rowSpacing = 5;
    const int startY = listArea.y + 28;

    if (games.empty()) {
        SDL_Rect r{};
        SDL_Texture* t = makeText("No hay partidas disponibles. Crea una nueva abajo.",
                                   fontSmall, C_DIM, r);
        if (t) {
            r.x = listArea.x + (listArea.w - r.w) / 2;
            r.y = startY + 20;
            drawTex(t, r); SDL_DestroyTexture(t);
        }
        return;
    }

    for (int i = 0; i < static_cast<int>(games.size()); ++i) {
        bool selected = (i == selectedGame);
        bool hovered  = (i == hoveredGame);
        bool full     = (games[i].players >= games[i].maxPlayers);

        SDL_Rect row = {listArea.x, startY + i * (rowH + rowSpacing),
                        listArea.w, rowH};

        // Fondo de fila
        SDL_Color bg;
        if (selected)     bg = C_SEL_BG;
        else if (hovered) bg = C_HOV_BG;
        else              bg = (i % 2 == 0) ? C_ROW_EVEN : C_ROW_ODD;
        drawFilledRoundRect(row, bg, 4);

        SDL_Color border = selected ? C_SEL : (hovered ? C_HOV : C_ROW_BORD);
        drawBorderRoundRect(row, border, 4);

        if (selected) {
            SDL_Rect bar = {row.x, row.y + 6, 3, row.h - 12};
            drawRect(bar, C_SEL, true);
        }

        // Nombre de la partida
        SDL_Color nameColor = selected ? C_SEL : (hovered ? C_HOV : C_TEXT);
        SDL_Rect nr{};
        SDL_Texture* nt = makeText(games[i].name, fontMedium, nameColor, nr);
        if (nt) {
            nr.x = row.x + 14;
            nr.y = row.y + (row.h - nr.h) / 2;
            drawTex(nt, nr); SDL_DestroyTexture(nt);
        }

        // Jugadores
        std::string slots = std::to_string(games[i].players) + " / " +
                            std::to_string(games[i].maxPlayers);
        SDL_Rect sr{};
        SDL_Texture* st = makeText(slots, fontSmall, C_DIM, sr);
        if (st) {
            sr.x = row.x + row.w - 185;
            sr.y = row.y + (row.h - sr.h) / 2;
            drawTex(st, sr); SDL_DestroyTexture(st);
        }

        // Badge de estado
        const char* badge = full ? "LLENA" : "LIBRE";
        SDL_Color badgeC  = full ? C_FULL : C_AVAIL;
        SDL_Rect br{};
        SDL_Texture* bt = makeText(badge, fontSmall, badgeC, br);
        if (bt) {
            SDL_Rect badgeBg = {row.x + row.w - 88, row.y + (row.h - br.h - 6) / 2,
                                br.w + 12, br.h + 6};
            SDL_Color bbg = full ? SDL_Color{60, 15, 15, 160} : SDL_Color{15, 50, 25, 160};
            drawFilledRoundRect(badgeBg, bbg, 3);
            drawBorderRoundRect(badgeBg, badgeC, 3);
            br.x = badgeBg.x + (badgeBg.w - br.w) / 2;
            br.y = badgeBg.y + (badgeBg.h - br.h) / 2;
            drawTex(bt, br); SDL_DestroyTexture(bt);
        }
    }
}

void PlaceholderLobbyScreen::renderCreateSection() {
    // Etiqueta sección
    SDL_Rect lr{};
    SDL_Texture* lt = makeText("NUEVA PARTIDA", fontSmall, C_SUBTITLE, lr);
    if (lt) { lr.x = createArea.x; lr.y = createArea.y + 4; drawTex(lt, lr); SDL_DestroyTexture(lt); }

    // Caja de input
    SDL_Rect inputBox = {createArea.x, createArea.y + 28,
                         createArea.w - 120, 34};
    SDL_Color inputBg = typingNewGame ? C_INPUT_ACT : C_INPUT_BG;
    drawFilledRoundRect(inputBox, inputBg, 4);
    SDL_Color inputBorder = typingNewGame ? C_SEL : C_ROW_BORD;
    drawBorderRoundRect(inputBox, inputBorder, 4);

    // Texto del input + cursor
    std::string display = newGameName;
    if (typingNewGame && (SDL_GetTicks() / 500) % 2 == 0) display += "|";
    if (display.empty() && !typingNewGame) display = "Nombre de la partida...";

    SDL_Color textColor = (!newGameName.empty() || typingNewGame) ? C_TEXT : C_DIM;
    SDL_Rect txtr{};
    SDL_Texture* txt = makeText(display, fontSmall, textColor, txtr);
    if (txt) {
        txtr.x = inputBox.x + 10;
        txtr.y = inputBox.y + (inputBox.h - txtr.h) / 2;
        drawTex(txt, txtr); SDL_DestroyTexture(txt);
    }

    // Botón Crear
    SDL_Rect bCreate = {createArea.x + createArea.w - 110,
                        createArea.y + 28, 100, 34};
    drawButton(bCreate, "Crear", C_BTN_CRE, C_AVAIL, hoverCreate);
}

void PlaceholderLobbyScreen::renderStatusBar() {
    // Botones principales
    const int bw = (buttonArea.w - 20) / 3;
    SDL_Rect bJoin    = {buttonArea.x,             buttonArea.y, bw, buttonArea.h};
    SDL_Rect bRefresh = {buttonArea.x + bw + 10,   buttonArea.y, bw, buttonArea.h};
    SDL_Rect bBack    = {buttonArea.x + (bw+10)*2, buttonArea.y, bw, buttonArea.h};

    drawButton(bJoin,    "Unirse",    C_BTN_JOIN, {60, 120, 200, 255}, hoverJoin);
    drawButton(bRefresh, "Refrescar", C_BTN_REF,  {140, 140, 60, 255}, hoverRefresh);
    drawButton(bBack,    "Volver",    C_BTN_BACK, {180, 60,  60, 255}, hoverBack);

    // Barra de mensajes (error o status) en la parte inferior del panel
    const int msgY = panel.y + panel.h + 8;
    if (!errorMsg.empty()) {
        SDL_Rect r{};
        SDL_Texture* t = makeText(errorMsg, fontSmall, C_ERROR, r);
        if (t) { r.x = windowW/2 - r.w/2; r.y = msgY; drawTex(t, r); SDL_DestroyTexture(t); }
    } else if (!statusMsg.empty()) {
        SDL_Rect r{};
        SDL_Texture* t = makeText(statusMsg, fontSmall, C_STATUS, r);
        if (t) { r.x = windowW/2 - r.w/2; r.y = msgY; drawTex(t, r); SDL_DestroyTexture(t); }
    }
}


// Helpers de dibujo
void PlaceholderLobbyScreen::drawFilledRoundRect(const SDL_Rect& r, SDL_Color color, int radius) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    // Rectángulo central
    SDL_Rect center = {r.x + radius, r.y, r.w - radius*2, r.h};
    SDL_RenderFillRect(renderer, &center);
    SDL_Rect left  = {r.x, r.y + radius, radius, r.h - radius*2};
    SDL_Rect right = {r.x + r.w - radius, r.y + radius, radius, r.h - radius*2};
    SDL_RenderFillRect(renderer, &left);
    SDL_RenderFillRect(renderer, &right);

    // Esquinas aproximadas como círculos de 1px
    for (int dy = 0; dy < radius; dy++) {
        int dx = static_cast<int>(std::sqrt(static_cast<float>(radius*radius - dy*dy)));
        // Top-left
        SDL_RenderDrawLine(renderer, r.x + radius - dx, r.y + dy,
                           r.x + radius,              r.y + dy);
        // Top-right
        SDL_RenderDrawLine(renderer, r.x + r.w - radius, r.y + dy,
                           r.x + r.w - radius + dx - 1, r.y + dy);
        // Bottom-left
        SDL_RenderDrawLine(renderer, r.x + radius - dx, r.y + r.h - 1 - dy,
                           r.x + radius,              r.y + r.h - 1 - dy);
        // Bottom-right
        SDL_RenderDrawLine(renderer, r.x + r.w - radius, r.y + r.h - 1 - dy,
                           r.x + r.w - radius + dx - 1, r.y + r.h - 1 - dy);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void PlaceholderLobbyScreen::drawBorderRoundRect(const SDL_Rect& r, SDL_Color color, int radius) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    // Lados
    SDL_RenderDrawLine(renderer, r.x + radius, r.y,          r.x + r.w - radius, r.y);
    SDL_RenderDrawLine(renderer, r.x + radius, r.y + r.h - 1, r.x + r.w - radius, r.y + r.h - 1);
    SDL_RenderDrawLine(renderer, r.x,          r.y + radius, r.x,                 r.y + r.h - radius);
    SDL_RenderDrawLine(renderer, r.x + r.w - 1, r.y + radius, r.x + r.w - 1,      r.y + r.h - radius);

    // Esquinas
    for (int dy = 0; dy < radius; dy++) {
        int dx = static_cast<int>(std::sqrt(static_cast<float>(radius*radius - dy*dy)));
        SDL_RenderDrawPoint(renderer, r.x + radius - dx,            r.y + dy);
        SDL_RenderDrawPoint(renderer, r.x + r.w - radius + dx - 1,  r.y + dy);
        SDL_RenderDrawPoint(renderer, r.x + radius - dx,            r.y + r.h - 1 - dy);
        SDL_RenderDrawPoint(renderer, r.x + r.w - radius + dx - 1,  r.y + r.h - 1 - dy);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void PlaceholderLobbyScreen::drawButton(const SDL_Rect& r, const char* label,
                                         SDL_Color bg, SDL_Color border, bool hovered) {
    SDL_Color activeBg = hovered
        ? SDL_Color{static_cast<Uint8>(std::min(bg.r + 30, 255)),
                    static_cast<Uint8>(std::min(bg.g + 30, 255)),
                    static_cast<Uint8>(std::min(bg.b + 30, 255)), bg.a}
        : bg;
    drawFilledRoundRect(r, activeBg, 5);
    drawBorderRoundRect(r, hovered ? C_HOV : border, 5);

    SDL_Rect tr{};
    SDL_Color labelColor = hovered ? C_HOV : C_TEXT;
    SDL_Texture* t = makeText(label, fontSmall, labelColor, tr);
    if (t) {
        tr.x = r.x + (r.w - tr.w) / 2;
        tr.y = r.y + (r.h - tr.h) / 2;
        drawTex(t, tr); SDL_DestroyTexture(t);
    }
}

SDL_Texture* PlaceholderLobbyScreen::makeText(const std::string& text, TTF_Font* font,
                                               SDL_Color color, SDL_Rect& out) {
    if (text.empty()) return nullptr;
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
    else      SDL_RenderDrawRect(renderer, &r);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

const PlayerDto& PlaceholderLobbyScreen::getJoinedPlayerDto() const {
    return joinedPlayerDto;
}
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <deque>

#include "common/network/messages/server/chat/chatNotificationMessage.h"

// Layout del área de chat: {0, 33, 900, 100}
//
//  y=33  ┌──────────────────────────────────────┐
//        │  línea 0  (y=36)                     │
//        │  línea 1  (y=50)                     │
//        │  línea 2  (y=64)                     │
//        │  línea 3  (y=78)                     │  ← máx 4 líneas visibles
//  y=93  ├──────────────────────────────────────┤
//        │  [ input box           ]  (y=95~118) │
//  y=133 └──────────────────────────────────────┘

class MiniChat
{
public:
    static SDL_Color colorFor(ChatMsgType type);

    void appendLine(const std::string &text, ChatMsgType type);

    bool handleEvent(const SDL_Event &event);

    bool hasPendingInput() const;
    std::string consumeInput();

    void render(SDL_Renderer *renderer, TTF_Font *font) const;

    void setFocused(bool focused);
    bool isFocused() const;

private:
    static constexpr int CHAT_MAX_LINES = 4;
    static constexpr int CHAT_MAX_INPUT = 120;
    static constexpr int LINE_H = 14;

    static constexpr int LOG_X = 8;
    static constexpr int LOG_Y = 36;
    static constexpr int LOG_W = 880;

    static constexpr int INPUT_X = 8;
    static constexpr int INPUT_Y = 109;
    static constexpr int INPUT_W = 868;
    static constexpr int INPUT_H = 20;

    struct LogLine
    {
        std::string text;
        ChatMsgType type;
    };

    std::deque<LogLine> lines;
    std::string inputBuffer;
    bool focused = false;
    bool pendingInput = false;
    std::string pendingText;

    void renderText(SDL_Renderer *renderer, TTF_Font *font,
                    const std::string &text, SDL_Color color,
                    int x, int y, int maxW) const;
};
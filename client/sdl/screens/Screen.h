#pragma once

#include <SDL2/SDL.h>

enum class ScreenResult {
    QUIT,
    GO_CREATE_CHAR,
    GO_LOGIN,
    GO_CONFIG,
    GO_LOBBY,
    GO_MAIN_MENU,
};

class Screen {
public:
    virtual ~Screen() = default;
    virtual ScreenResult run() = 0;
};

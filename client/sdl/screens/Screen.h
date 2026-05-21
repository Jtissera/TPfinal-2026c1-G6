#pragma once

#include <SDL2/SDL.h>

// Resultado que retorna cada pantalla al terminar su loop
enum class ScreenResult {
    QUIT,            // El usuario cerró la ventana o eligió Salir
    GO_CREATE_CHAR,  // Ir a crear personaje
    GO_LOGIN,        // Ir a iniciar sesión con personaje existente
    GO_CONFIG,       // Ir a configuración
    GO_LOBBY,        // Ir al lobby (post login/creación exitosa)
};

class Screen {
public:
    virtual ~Screen() = default;

    // Corre el loop completo de esta pantalla y devuelve qué hacer después
    virtual ScreenResult run() = 0;
};
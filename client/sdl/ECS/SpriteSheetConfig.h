
#ifndef TALLER_TP_SPRITESHEETCONFIG_H
#define TALLER_TP_SPRITESHEETCONFIG_H


#pragma once

// Configuración básica de un spritesheet.
// Sirve para decirle al SpriteComponent cuánto mide cada frame real.
struct SpriteSheetConfig {
    int frameWidth;   // Ancho real de cada frame dentro del spritesheet.
    int frameHeight;  // Alto real de cada frame dentro del spritesheet.
    int scale;        // Escala visual al dibujar en pantalla.
};



#endif //TALLER_TP_SPRITESHEETCONFIG_H

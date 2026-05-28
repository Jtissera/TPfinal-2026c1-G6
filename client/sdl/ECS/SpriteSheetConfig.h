
#ifndef TALLER_TP_SPRITESHEETCONFIG_H
#define TALLER_TP_SPRITESHEETCONFIG_H


#pragma once

// Configuración básica de un spritesheet.
// Sirve para decirle al SpriteComponent cuánto mide cada frame real.
struct SpriteSheetConfig {
    int frameWidth;
    int frameHeight;
    int scale;

    int startX = 0;
    int startY = 0;
};



#endif //TALLER_TP_SPRITESHEETCONFIG_H

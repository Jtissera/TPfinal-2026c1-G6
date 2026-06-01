
#ifndef TALLER_TP_SPRITESHEETCONFIG_H
#define TALLER_TP_SPRITESHEETCONFIG_H


#pragma once
// Configuración básica de un spritesheet.
// Define cómo recortar el sprite y cómo ajustarlo visualmente al dibujarlo.
struct SpriteSheetConfig {
    // Ancho real de cada frame dentro del spritesheet.
    int frameWidth;

    // Alto real de cada frame dentro del spritesheet.
    int frameHeight;

    // Escala visual usada al dibujar el sprite.
    int scale;

    // Coordenada X inicial dentro del archivo de imagen.
    int startX = 0;

    // Coordenada Y inicial dentro del archivo de imagen.
    int startY = 0;

    // Ajuste horizontal visual.
    // Se usa para corregir sprites que no calzan perfecto.
    int renderOffsetX = 0;

    // Ajuste vertical visual.
    // Negativo = sube el sprite.
    // Positivo = baja el sprite.
    int renderOffsetY = 0;
};

#endif //TALLER_TP_SPRITESHEETCONFIG_H

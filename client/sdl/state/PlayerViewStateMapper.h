//
#ifndef TALLER_TP_PLAYERVIEWSTATEMAPPER_H
#define TALLER_TP_PLAYERVIEWSTATEMAPPER_H


#pragma once

#include "PlayerViewState.h"
#include "common/dtos/gameTypes.h"

// Convierte el DTO recibido por protocolo en el estado visual que usa SDL.
inline PlayerViewState toPlayerViewState(const PlayerDto& dto) {
    PlayerViewState state;

    state.name = dto.nombre;
    state.race = dto.raza;
    state.playerClass = dto.clase;

    state.level = dto.level;

    state.hp = dto.hp;
    state.maxHp = dto.hpMax;

    state.mana = dto.mana;
    state.maxMana = dto.manaMax;

    state.exp = dto.exp;
    state.expToNextLevel = dto.expMax;

    state.gold = dto.oro;

    state.isDead = dto.esFantasma;

    return state;
}
#endif //TALLER_TP_PLAYERVIEWSTATEMAPPER_H

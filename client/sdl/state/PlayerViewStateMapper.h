//
#ifndef TALLER_TP_PLAYERVIEWSTATEMAPPER_H
#define TALLER_TP_PLAYERVIEWSTATEMAPPER_H


#pragma once

#include "PlayerViewState.h"
#include "common/dtos/gameTypes.h"

inline PlayerClass parsePlayerClass(const std::string& value) {
    if (value == "Warrior") {
        return PlayerClass::Warrior;
    }

    if (value == "Mage") {
        return PlayerClass::Mage;
    }

    if (value == "Paladin") {
        return PlayerClass::Paladin;
    }

    if (value == "Cleric") {
        return PlayerClass::Cleric;
    }

    return PlayerClass::Unknown;
}

// Convierte el DTO recibido por protocolo en el estado visual que usa SDL.
inline PlayerViewState toPlayerViewState(const PlayerDto& dto) {
    PlayerViewState state;

    state.name = dto.nombre;
    std::cout << "[CLIENT DTO] raza='" << dto.raza << "'" << std::endl;
    state.race = dto.raza;
    std::cout << "[CLIENT DTO] clase='" << dto.clase << "'" << std::endl;
    state.playerClass = parsePlayerClass(dto.clase);


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

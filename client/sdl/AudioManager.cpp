#include "AudioManager.h"
#include <iostream>

AudioManager::~AudioManager()
{
    for (auto& [id, chunk] : effects)
    {
        if (chunk != nullptr)
            Mix_FreeChunk(chunk);
    }
    effects.clear();

    if (music != nullptr)
    {
        Mix_FreeMusic(music);
        music = nullptr;
    }
}

void AudioManager::loadMusic(const std::string& path)
{
    // Verificar si SDL_mixer está disponible.
    if (Mix_QuerySpec(nullptr, nullptr, nullptr) == 0)
    {
        available = false;
        return;
    }
    available = true;

    if (music != nullptr)
    {
        Mix_FreeMusic(music);
        music = nullptr;
    }

    music = Mix_LoadMUS(path.c_str());
    if (music == nullptr)
        std::cerr << "[Audio] No se pudo cargar música: " << path
                  << " — " << Mix_GetError() << "\n";
}

void AudioManager::loadEffect(const std::string& id, const std::string& path)
{
    if (!available)
        return;

    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (chunk == nullptr)
    {
        std::cerr << "[Audio] No se pudo cargar efecto '" << id
                  << "': " << path << " — " << Mix_GetError() << "\n";
        return;
    }

    // Liberar si ya existía con ese id.
    auto it = effects.find(id);
    if (it != effects.end() && it->second != nullptr)
        Mix_FreeChunk(it->second);

    effects[id] = chunk;
}

void AudioManager::playMusic()
{
    if (!available || music == nullptr)
        return;

    if (Mix_PlayMusic(music, -1) == -1)
        std::cerr << "[Audio] Mix_PlayMusic: " << Mix_GetError() << "\n";
}

void AudioManager::stopMusic()
{
    Mix_HaltMusic();
}

void AudioManager::playEffect(const std::string& id)
{
    if (!available)
        return;

    auto it = effects.find(id);
    if (it == effects.end() || it->second == nullptr)
        return;

    // Volumen ya seteado en el chunk via setSfxVolume.
    Mix_PlayChannel(-1, it->second, 0);
}

void AudioManager::setMusicVolume(int volume0to100)
{
    const int vol = (volume0to100 * MIX_MAX_VOLUME) / 100;
    Mix_VolumeMusic(vol);
}

void AudioManager::setSfxVolume(int volume0to100)
{
    sfxVolume = (volume0to100 * MIX_MAX_VOLUME) / 100;

    // Aplicar a todos los chunks ya cargados.
    for (auto& [id, chunk] : effects)
    {
        if (chunk != nullptr)
            Mix_VolumeChunk(chunk, sfxVolume);
    }

    // Aplicar a todos los canales activos
    Mix_Volume(-1, sfxVolume);
}
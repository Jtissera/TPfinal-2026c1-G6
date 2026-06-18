#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <SDL2/SDL_mixer.h>
#include <string>
#include <unordered_map>

// AudioManager — carga y reproduce música y efectos de sonido con SDL_mixer.
// El volumen de música y efectos se controla con setMusicVolume / setSfxVolume,
// que reciben valores de 0 a 100 (igual que los sliders de ConfigScreen).

class AudioManager {
public:
    AudioManager() = default;
    ~AudioManager();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // Carga un archivo OGG/MP3 como musica de fondo (solo uno activo a la vez).
    // Llama antes de playMusic(). Si falla, el juego sigue sin músmusicaica.
    void loadMusic(const std::string& path);

    // Carga un efecto de sonido con un id de nombre.
    // Ejemplo: loadEffect("attack", "assets/audio/SoundsOgg/8.ogg")
    void loadEffect(const std::string& id, const std::string& path);

    // Reproduce la musica cargada en loop. No hace nada si no se cargó musica.
    void playMusic();

    // Detiene la musica.
    void stopMusic();

    // Reproduce un efecto por id. Si el id no existe o el audio no está
    // disponible, no hace nada (el juego no crashea).
    void playEffect(const std::string& id);

    // Volumen de musica: 0-100.
    void setMusicVolume(int volume0to100);

    // Volumen de efectos: 0-100.
    void setSfxVolume(int volume0to100);

    // True si SDL_mixer se inicializo correctamente.
    bool isAvailable() const { return available; }

private:
    bool available = false;

    Mix_Music* music = nullptr;

    // Cache de efectos de sonido cargados.
    std::unordered_map<std::string, Mix_Chunk*> effects;

    int sfxVolume = MIX_MAX_VOLUME;
};

#endif // AUDIO_MANAGER_H
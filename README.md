# Argentum — TP Taller de Programación I (FIUBA)

## Estructura del proyecto

```
argentum-online/
├── client/       # Cliente grafico (SDL2 + SDL2pp)
├── server/       # Servidor del juego (sockets POSIX)
├── editor/       # Editor de mapas (Qt)
├── common/       # Librería estatica compartida
├── tests/        # Unit tests (GoogleTest)
├── assets/
│   ├── sprites/  # Sprites y tilesets
│   ├── audio/    # Musica y efectos de sonido
│   └── fonts/    # Fuentes tipográficas
├── config/       # Configuración TOML del juego
├── docs/         # Documentación técnica y de usuario
├── cmake/        # Modulos CMake auxiliares
├── CMakeLists.txt
└── Makefile
```

## Dependencias del sistema

```bash
sudo apt-get install \
    libopus-dev libopusfile-dev \
    libxmp-dev libfluidsynth-dev fluidsynth \
    libwavpack1 libwavpack-dev \
    libfreetype-dev wavpack \
    qtbase5-dev
```

## Compilar y ejecutar

```bash
make build       # Compila todo
make test        # Compila y corre los tests
make run-server  # Ejecuta el servidor
make run-client  # Ejecuta el cliente
make run-editor  # Ejecuta el editor
make help        # Ver todos los targets
```

## Equipo:

# Argentum — TP Taller de Programación I (FIUBA)

## Estructura del proyecto


```
argentum-online/
├── client/                   # Cliente gráfico (SDL2 + SDL2pp)
│   ├── network/              # Comunicación con el servidor
│   ├── sdl/
│   │   ├── ECS/              # Entity Component System
│   │   ├── screens/          # Menús y pantallas
│   │   ├── world/            # Representación del mundo cliente
│   │   ├── items/            # Catálogo visual de ítems
│   │   └── state/            # Estado visual del jugador
│   ├── GameClient.*          # Lógica principal del cliente
│   └── Game.*                # Renderizado y actualización del juego
│
├── server/                   # Servidor autoritativo
│   ├── network/              # Sockets, protocolos y manejo de clientes
│   ├── game/
│   │   ├── player/           # Jugadores e inventarios
│   │   ├── combat/           # Sistema de combate
│   │   ├── items/            # Ítems y equipamiento
│   │   ├── clan/             # Gestión de clanes
│   │   ├── chat/             # Comandos y chat global
│   │   ├── session/          # Game loop y administración de partidas
│   │   └── stats/            # Razas, clases y fórmulas
│   ├── world/               # Mundo, colisiones y spawns
│   ├── npc/                 # NPCs e IA
│   ├── persistence/         
│   ├── lobby/               
│   ├── city/                
│   ├── bank/                
│   └── resurrection/        
│
├── editor/                  # Editor de mapas (Qt)
│   ├── map/                 # Modelo y serialización de mapas
│   └── widgets/             # Canvas y paletas de edición
│
├── common/                  # Código compartido cliente-servidor
├── tests/                   # Tests unitarios
│
├── assets/
│   ├── sprites/             # Sprites y tilesets
│   ├── audio/               # Música y efectos
│   └── fonts/               # Fuentes
│
├── config/                  # Configuración TOML
├── docs/                    # Documentación técnica y de usuario
├── cmake/                   # Módulos auxiliares de CMake
├── CMakeLists.txt
└── Makefile
```

## Dependencias del sistema

```bash
sudo apt-get install -y \
    git \
    cmake \
    g++ \
    make \
    build-essential \
    valgrind \
    qtbase5-dev \
    qt5-qmake \
    "$QT_WIDGETS_PKG" \
    libopus-dev \
    libopusfile-dev \
    libxmp-dev \
    libwavpack-dev \
    wavpack \
    libfreetype-dev \
    libjpeg-dev \
    libpng-dev \
    nlohmann-json3-dev \
    xdg-user-dirs \
    libmikmod-dev \
    libmpg123-dev \
    libvorbis-dev \
    libogg-dev \
    libpulse-dev \
    libasound2-dev \
    libsdl2-ttf-dev \
    libsdl2-image-dev \
    libsdl2-mixer-dev

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

## Equipo

- Jose Ignacio Adelardi - 111701
- Jose Evaristo Tissera - 112788
- Camila Miranda Vandevalle - 112777
- Maurizio Andres Giannantonio Reina - 108119
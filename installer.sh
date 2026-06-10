#!/bin/bash
set -e

# ==============================================================================
#  Argentum Online — Installer
# ==============================================================================

GREEN='\033[0;32m'
BLUE='\033[94m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

REPO_URL="https://github.com/Jtissera/TPfinal-2026c1-G6.git"
BRANCH="dev"
GAME_NAME="argentum"
SRC_DIR="/tmp/${GAME_NAME}_src"
INSTALL_DIR="/var/$GAME_NAME"
CONFIG_DIR="/etc/$GAME_NAME"
BIN_DIR="/usr/bin"

echo ""
echo -e "${BLUE}=====================================================${NC}"
echo -e "${BLUE}         Argentum Online — Instalador               ${NC}"
echo -e "${BLUE}=====================================================${NC}"
echo ""

# ------------------------------------------------------------------------------
# 1. Dependencias del sistema
# ------------------------------------------------------------------------------
echo -e "${YELLOW}[1/5] Instalando dependencias del sistema...${NC}"

UBUNTU_VERSION=$(lsb_release -rs 2>/dev/null || echo "0")
if dpkg --compare-versions "$UBUNTU_VERSION" ge "24.04" 2>/dev/null; then
    QT_WIDGETS_PKG="libqt5widgets5t64"
else
    QT_WIDGETS_PKG="libqt5widgets5"
fi

sudo apt-get update -qq
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

# FIX: detectar desktop DESPUÉS de instalar xdg-user-dirs
DESKTOP_DIR="$(xdg-user-dir DESKTOP 2>/dev/null || echo "$HOME/Desktop")"
if [ ! -d "$DESKTOP_DIR" ]; then
    DESKTOP_DIR="$HOME/Desktop"
    mkdir -p "$DESKTOP_DIR"
fi

echo -e "${GREEN}[1/5] Dependencias instaladas.${NC}"

# ------------------------------------------------------------------------------
# 2. Clonar / actualizar repositorio
# ------------------------------------------------------------------------------
echo -e "${YELLOW}[2/5] Descargando código fuente...${NC}"

if [ -d "$SRC_DIR/.git" ]; then
    echo "      Repositorio ya existe, actualizando..."
    cd "$SRC_DIR"
    git fetch origin
    git checkout "$BRANCH"
    git pull origin "$BRANCH"
else
    git clone --branch "$BRANCH" "$REPO_URL" "$SRC_DIR"
    cd "$SRC_DIR"
fi

echo -e "${GREEN}[2/5] Código fuente listo.${NC}"

# ------------------------------------------------------------------------------
# 3. Compilar (SDL2 se descarga de GitHub en este paso (~300MB). Puede tardar 15 min.)
# ------------------------------------------------------------------------------
echo -e "${YELLOW}[3/5] Compilando (puede tardar 15-20 minutos, SDL se descarga de GitHub)...${NC}"

cd "$SRC_DIR"
rm -rf build
mkdir -p build
cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DTALLER_EDITOR=OFF \
    -DTALLER_TESTS=OFF \
    -DTALLER_MAKE_WARNINGS_AS_ERRORS=OFF \
    -DSDL_PIPEWIRE=OFF \
    -DSDL2MIXER_MIDI_FLUIDSYNTH=OFF
cmake --build . -- -j"$(nproc)"
cd "$SRC_DIR"

echo -e "${GREEN}[3/5] Compilación exitosa.${NC}"

# ------------------------------------------------------------------------------
# 4. Instalar binarios, assets y configs
# ------------------------------------------------------------------------------
echo -e "${YELLOW}[4/5] Instalando archivos...${NC}"

# Binarios → /usr/bin
sudo cp "$SRC_DIR/build/taller_server" "$BIN_DIR/${GAME_NAME}_server"
sudo cp "$SRC_DIR/build/taller_client" "$BIN_DIR/${GAME_NAME}_client"
sudo chmod +x "$BIN_DIR/${GAME_NAME}_server"
sudo chmod +x "$BIN_DIR/${GAME_NAME}_client"

# Data files (assets) → /var/argentum
sudo mkdir -p "$INSTALL_DIR"
sudo cp -r "$SRC_DIR/assets" "$INSTALL_DIR/"
sudo chmod -R 755 "$INSTALL_DIR"

# Config → /etc/argentum
sudo mkdir -p "$CONFIG_DIR"
sudo cp "$SRC_DIR/config/game.toml" "$CONFIG_DIR/game.toml"
sudo chmod 644 "$CONFIG_DIR/game.toml"

# Copia en /var/argentum/config/ para que el servidor encuentre
# "config/game.toml" con el path relativo que usa internamente
sudo mkdir -p "$INSTALL_DIR/config"
sudo cp "$SRC_DIR/config/game.toml" "$INSTALL_DIR/config/game.toml"

# Scripts de lanzamiento → Desktop
mkdir -p "$DESKTOP_DIR"
cp "$SRC_DIR/server.sh" "$DESKTOP_DIR/server.sh"
cp "$SRC_DIR/client.sh" "$DESKTOP_DIR/client.sh"
chmod +x "$DESKTOP_DIR/server.sh"
chmod +x "$DESKTOP_DIR/client.sh"

echo -e "${GREEN}[4/5] Archivos instalados.${NC}"

# ------------------------------------------------------------------------------
# 5. Verificar binarios
# ------------------------------------------------------------------------------
echo -e "${YELLOW}[5/5] Verificando instalación...${NC}"

if [ -f "$BIN_DIR/${GAME_NAME}_server" ] && [ -f "$BIN_DIR/${GAME_NAME}_client" ]; then
    echo -e "${GREEN}[5/5] Binarios instalados correctamente.${NC}"
else
    echo -e "${RED}[5/5] Error: no se encontraron los binarios instalados.${NC}"
    exit 1
fi

# ------------------------------------------------------------------------------
# Resumen final
# ------------------------------------------------------------------------------
echo ""
echo -e "${BLUE}=====================================================${NC}"
echo -e "${GREEN}  ¡Instalación completa!${NC}"
echo -e "${BLUE}=====================================================${NC}"
echo ""
echo -e "  Binarios en:        ${BLUE}$BIN_DIR${NC}"
echo -e "  Assets en:          ${BLUE}$INSTALL_DIR${NC}"
echo -e "  Configuración en:   ${BLUE}$CONFIG_DIR${NC}"
echo -e "  Scripts en:         ${BLUE}$DESKTOP_DIR${NC}"
echo -e "                      (server.sh y client.sh)"
echo ""
echo -e "  ${YELLOW}Para jugar:${NC}"
echo -e "  1. Abrir una terminal y correr: ${GREEN}$DESKTOP_DIR/server.sh${NC}"
echo -e "  2. Abrir otra terminal y correr: ${GREEN}$DESKTOP_DIR/client.sh${NC}"
echo ""
echo -e "  Para cambiar el puerto, editar los scripts del escritorio."
echo ""
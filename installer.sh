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
BRANCH="main"
GAME_NAME="argentum"
SRC_DIR="/tmp/${GAME_NAME}_src"
INSTALL_DIR="/var/$GAME_NAME"
CONFIG_DIR="/etc/$GAME_NAME"
BIN_DIR="/usr/bin"
DESKTOP_DIR="$HOME/Desktop"

echo ""
echo -e "${BLUE}=====================================================${NC}"
echo -e "${BLUE}         Argentum Online — Instalador               ${NC}"
echo -e "${BLUE}=====================================================${NC}"
echo ""

# ------------------------------------------------------------------------------
# 1. Dependencias del sistema
# ------------------------------------------------------------------------------
echo -e "${YELLOW}[1/5] Instalando dependencias del sistema...${NC}"

sudo apt-get update -qq
sudo apt-get install -y \
    git \
    cmake \
    g++ \
    make \
    build-essential \
    valgrind \
    libsdl2-dev \
    libsdl2-image-dev \
    libsdl2-mixer-dev \
    libsdl2-ttf-dev \
    qtbase5-dev \
    qt5-qmake \
    libqt5widgets5 \
    libopus-dev \
    libopusfile-dev \
    libxmp-dev \
    libwavpack-dev \
    wavpack \
    libfreetype-dev \
    libjpeg-dev \
    libpng-dev \
    nlohmann-json3-dev

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
# 3. Compilar
# ------------------------------------------------------------------------------
echo -e "${YELLOW}[3/5] Compilando (esto puede tardar unos minutos)...${NC}"

mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
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
sudo cp "$SRC_DIR/build/taller_editor" "$BIN_DIR/${GAME_NAME}_editor"
sudo chmod +x "$BIN_DIR/${GAME_NAME}_server"
sudo chmod +x "$BIN_DIR/${GAME_NAME}_client"
sudo chmod +x "$BIN_DIR/${GAME_NAME}_editor"

# Data files (assets) → /var/argentum
sudo mkdir -p "$INSTALL_DIR"
sudo cp -r "$SRC_DIR/assets" "$INSTALL_DIR/"
sudo chmod -R 755 "$INSTALL_DIR"

# Config → /etc/argentum  (canónico)
sudo mkdir -p "$CONFIG_DIR"
sudo cp "$SRC_DIR/config/game.toml" "$CONFIG_DIR/game.toml"
sudo chmod 644 "$CONFIG_DIR/game.toml"

# Copia en /var/argentum/config/ para que el servidor encuentre
# "config/game.toml" con el path relativo que ya usa internamente
sudo mkdir -p "$INSTALL_DIR/config"
sudo cp "$SRC_DIR/config/game.toml" "$INSTALL_DIR/config/game.toml"

# Scripts de lanzamiento del repo → Desktop
# (la correctora los pide explícitamente desde el repo)
mkdir -p "$DESKTOP_DIR"
cp "$SRC_DIR/server.sh" "$DESKTOP_DIR/server.sh"
cp "$SRC_DIR/client.sh" "$DESKTOP_DIR/client.sh"
chmod +x "$DESKTOP_DIR/server.sh"
chmod +x "$DESKTOP_DIR/client.sh"

echo -e "${GREEN}[4/5] Archivos instalados.${NC}"

# ------------------------------------------------------------------------------
# 5. Correr tests
# ------------------------------------------------------------------------------
echo -e "${YELLOW}[5/5] Corriendo tests...${NC}"

cd "$SRC_DIR/build"
./taller_tests --gtest_filter="-GameManagerTest.*" && \
    echo -e "${GREEN}[5/5] Tests OK.${NC}" || \
    echo -e "${RED}[5/5] Algunos tests fallaron — revisar antes de la entrega.${NC}"
cd "$SRC_DIR"

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
echo ""
echo -e "  ${YELLOW}Para jugar:${NC}"
echo -e "  1. Abrir una terminal y correr: ${GREEN}~/Desktop/server.sh${NC}"
echo -e "  2. Abrir otra terminal y correr: ${GREEN}~/Desktop/client.sh${NC}"
echo ""
echo -e "  Para cambiar el puerto, editar los scripts del escritorio."
echo ""
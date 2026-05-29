#!/bin/bash
set -e

REPO_URL="https://github.com/Jtissera/TPfinal-2026c1-G6.git"
BRANCH="main"
INSTALL_DIR="$HOME/argentum"

echo "======================================"
echo "  Argentum Online - Installer"
echo "======================================"

# 1. Instalar git si no esta
if ! command -v git &>/dev/null; then
    echo "[1/5] Instalando git..."
    sudo apt-get update -qq
    sudo apt-get install -y git
else
    echo "[1/5] git ya instalado."
fi

# 2. Instalar dependencias
echo "[2/5] Instalando dependencias del sistema..."
sudo apt-get update -qq
sudo apt-get install -y \
    cmake \
    g++ \
    make \
    valgrind \
    libsdl2-dev \
    libsdl2-image-dev \
    libsdl2-mixer-dev \
    libsdl2-ttf-dev \
    qtbase5-dev \
    libopus-dev \
    libopusfile-dev \
    libxmp-dev \
    libwavpack-dev \
    wavpack \
    libfreetype-dev

echo "[2/5] Dependencias instaladas."

# 3. Clonar el repositorio
echo "[3/5] Clonando repositorio..."
if [ -d "$INSTALL_DIR/.git" ]; then
    echo "      Repositorio ya existe en $INSTALL_DIR, actualizando..."
    cd "$INSTALL_DIR"
    git fetch origin
    git checkout "$BRANCH"
    git pull origin "$BRANCH"
else
    git clone --branch "$BRANCH" "$REPO_URL" "$INSTALL_DIR"
    cd "$INSTALL_DIR"
fi

# 4. Compilar
echo "[4/5] Compilando (esto puede tardar unos minutos)..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -- -j"$(nproc)"
cd ..

echo "[4/5] Compilación exitosa."

# 5. Crear scripts de ejecucion 
echo "[5/5] Creando scripts de ejecución..."

cat > "$INSTALL_DIR/server_execute.sh" << 'EOF'
#!/bin/bash
cd "$(dirname "$0")"
./build/taller_server 8080
EOF
chmod +x "$INSTALL_DIR/server_execute.sh"

cat > "$INSTALL_DIR/client_execute.sh" << 'EOF'
#!/bin/bash
cd "$(dirname "$0")"
./build/taller_client localhost 8080
EOF
chmod +x "$INSTALL_DIR/client_execute.sh"

echo ""
echo "======================================"
echo "  Instalacion completa."
echo ""
echo "  Para levantar el servidor:"
echo "    $INSTALL_DIR/server_execute.sh"
echo ""
echo "  Para levantar el cliente:"
echo "    $INSTALL_DIR/client_execute.sh"
echo ""
echo "  (Arrancar primero el servidor, luego el cliente)"
echo "======================================"
#!/bin/bash
set -e

echo "Instalando dependencias del sistema para Argentum"

sudo apt-get update

sudo apt-get install -y \
    cmake \
    g++ \
    clang-format \
    cppcheck \
    valgrind \
    python3-pip \
    libopus-dev libopusfile-dev \
    libxmp-dev libfluidsynth-dev fluidsynth \
    libwavpack1 libwavpack-dev \
    libfreetype-dev wavpack \
    libsdl2-dev \
    libsdl2-image-dev \
    libsdl2-mixer-dev \
    libsdl2-ttf-dev \
    qtbase5-dev

pipx install pre-commit cpplint
pre-commit install

echo "Dependencias instaladas "
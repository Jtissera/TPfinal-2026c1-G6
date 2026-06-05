#!/bin/bash
# Argentum Online — Servidor
# Para cambiar el puerto, modificar la variable PORT de aca abajo
PORT=8080

cd /var/argentum
export ARGENTUM_CONFIG_FILE=""

/usr/bin/argentum_server $PORT
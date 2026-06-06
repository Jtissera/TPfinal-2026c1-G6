#!/bin/bash
# Argentum Online — Cliente
# Para cambiar el host o puerto, modificar las variables de aca abajo
HOST=localhost
PORT=8080

cd /var/argentum
export ARGENTUM_CLIENT_CONFIG_FILE="/etc/argentum/game.toml"

/usr/bin/argentum_client $HOST $PORT
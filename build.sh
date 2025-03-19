#!/bin/bash

# Определяем путь к директории, где находится этот скрипт
SCRIPT_DIR="$(dirname "$(realpath "$0")")"

# Загружаем переменные
source config.env

docker run --rm --cpus="$CPUS" --memory="$MEMORY"g -it \
    -v "$PWD:/usr/src/tdesktop" \
    tdesktop:centos_env \
    /usr/src/tdesktop/Telegram/build/docker/centos_env/build.sh \
    -D TDESKTOP_API_ID="$YOUR_API_ID" \
    -D TDESKTOP_API_HASH="$YOUR_API_HASH" \
    -D DB_PATH="$SCRIPT_DIR/$EXTENSION_DB_PATH"

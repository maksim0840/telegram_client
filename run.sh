#!/bin/bash

# Переменные
SH_FILE_DIR=$(pwd)
APP_RUN_DIR=$(realpath "$SH_FILE_DIR/out/Release/Telegram")

"$APP_RUN_DIR" &
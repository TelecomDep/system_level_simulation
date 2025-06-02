#!/usr/bin/env bash

PLAYBOOK_PATH="system_simulation_5g.yml"
INVENTORY_FILE="inventory.ini"
INSTALL_PATH="$(pwd)/build"
INSTALL_USER=$(whoami)

# Создание директории 
echo "Создание директории ..."
echo "$INSTALL_PATH"
echo "$INSTALL_USER"
# mkdir -p "$INSTALL_PATH"

# Установка Ansible
echo "Установка Ansible..."
if ! command -v ansible &> /dev/null; then
    sudo apt update
    sudo apt install -y ansible
else
    echo "Ansible уже установлен."
fi


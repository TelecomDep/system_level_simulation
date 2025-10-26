## Virtual 
 Проверено на 22.04.1 и 24.04 Ubuntu

# Установка
## Клонируем
```bash
git clone https://github.com/sibsutisTelecomDep/system_level_simulation.git
cd system_level_simulation
git submodule update --init --recursive
```

## Установка "всего" при помощи Ansible:

Запускаем скрипт `./install.sh`, который установит `Ansible` и далее установит все необходимые зависимости:

```bash
./install.sh
# Become pass: пароль от вашего sudo профиля
```
Также произойдет сборка и компиляция проектов:

```bash
# libzmq
# czmq
# jeromq
# QuaDRiGa
# srsRAN_4G - для запуска UE
# srsRAN_Project - для запуска ядра и gNb 5G
```

### Установка Docker
Необходима для запуска `Open5GC`, `Grafana`.
<!-- 1. Set up Docker's `apt` repo:
```bash
# Add Docker's official GPG key:
sudo apt-get update
sudo apt-get install ca-certificates curl
sudo install -m 0755 -d /etc/apt/keyrings
sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
sudo chmod a+r /etc/apt/keyrings/docker.asc

# Add the repository to Apt sources:
echo \
"deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu \
$(. /etc/os-release && echo "${UBUNTU_CODENAME:-$VERSION_CODENAME}") stable" | \
sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
sudo apt-get update
```

2. Install Doker Packages:

```bash
sudo apt-get install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
```
3. Verify Docker installed successfully by running `hello-world` container:

```bash
sudo docker run hello-world
```

Если не работает, устанавливаем по [ссылке](https://timeweb.cloud/tutorials/docker/kak-ustanovit-docker-na-ubuntu-22-04) -->

1. Обновляем индексы пакетов `apt`;
```
sudo apt update
```

2. Устанавливаем доп. пакеты;
```
sudo apt install curl software-properties-common ca-certificates apt-transport-https -y
```
3. Импортируем GPG-ключ;
```
wget -O- https://download.docker.com/linux/ubuntu/gpg | gpg --dearmor | sudo tee /etc/apt/keyrings/docker.gpg > /dev/null
```

4. Добавляем репозитрий докера
```
echo "deb [arch=amd64 signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu jammy stable"| sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
```

5. Снова обновляем индексы пакетов `apt`;
```
sudo apt update
```

6. Проверяем доступ к репозиторию;
```
apt-cache policy docker-ce
```

7. Устанавливаем докер и пр.
```
sudo apt-get install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
```

### Установка Matlab

Download Matlab for Linux (2025a).
Install:
```bash
sudo mkdir -p /media/iso
sudo mount -o ro,loop R2025a_Linux.iso /media/iso
cd /media/iso/
# Здесь необходимо отключить Интернет
sudo ./install 

# Здесь следуем рекоммендациям 
 
 # После установки "размонтировать" образ
 sudo umount /media/iso
```

# Запуск системы имитационного моделирования

1. Запускаем Open5GS и Grafana (если необходимо):
```bash
```
2. Запускаем `Matlab`-скрипт. 
На стороне `Matlab` запускается серверная часть (`socket.bind()`).

3. Далее, запускаем 
```
```













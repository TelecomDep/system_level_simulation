# ZeroMQ Multi-UE Radio Broker

Модульный C++ брокер радиоканала для system-level симуляций (srsRAN 4G / 5G Project) с поддержкой выноса расчета моделей канала в MATLAB через ZeroMQ Multipart сообщения.

## Структура проекта

- `CMakeLists.txt` — конфигурация сборки C++17 (ZeroMQ, VOLK, yaml-cpp).
- `config.yaml` — параметры радиоканала, топологии абонентов (порты, расстояния) и адрес MATLAB.
- `Config.hpp` — парсер конфигурации на базе `yaml-cpp`.
- `ChannelProtocol.hpp` — бинарный заголовок метаданных (16 байт) для обмена с MATLAB.
- `RadioEndpoint.hpp` / `.cpp` — класс радиоинтерфейса узла сети (gNB или UE) с кольцевыми буферами и управлением ZMQ-сокетами (RAII).
- `ZmqRadioBroker.hpp` / `.cpp` — класс брокера: раздача Downlink, суммирование Uplink с SIMD VOLK и перенаправление сэмплов в MATLAB.
- `main.cpp` — точка входа и цикл тактования брокера.
- `matlab/channel_server.m` — сервер расчета моделей 3GPP (nrTDLChannel, AWGN, Path Loss) на MATLAB.

## Сборка и запуск

1. Установка зависимостей в Ubuntu / WSL2:
   ```bash
   sudo apt-get update
   sudo apt-get install -y build-essential cmake libzmq3-dev libvolk2-dev libyaml-cpp-dev
   ```

2. Сборка:
   ```bash
   mkdir -p build && cd build
   cmake ..
   cmake --build . -j$(nproc)
   ```

3. Запуск:
   - Если используется MATLAB: запустите `matlab/channel_server.m` в MATLAB.
   - Запуск брокера:
     ```bash
     ./zmq_broker ../config.yaml
     ```

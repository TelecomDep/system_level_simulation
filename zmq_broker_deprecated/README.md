# Порядок запуска компонентов системы

### Запуск коры
В директории /docker проекта srsRAN_5G:

```sudo docker  compose  up  --build  5gc```

### Запуск базы
В директории /build/apps/gnb проекта srsRAN_5G:

```sudo  ./gnb  -c  gnb_zmq.yaml```

### Запуск пользователей
В директории /build/srsue/src проекта srsRAN_4G:

```sudo  ./srsue  ./ue1_zmq.conf```
```sudo  ./srsue  ./ue2_zmq.conf```

### Cборка и запуск брокера

В директории /zmq_broker проекта system_level_simulation:

```
mkdir build
cd build
cmake ..
make
./main
```

### Матлаб сервер

Файл с матлаб сервером в директории /matlab


### Примечание

Предполагается запуск с 1 базой и 2 пользователями. Для изменения кол-ва пользователей необходимо внести изменения в матлаб файл и конфиг брокера, который находится в
директории /configs

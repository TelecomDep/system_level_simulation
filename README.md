## Proxy server между srsenb и srsue

### Проверено на 22.04.1 и 24.04 Ubuntu

### Установка компонентов:

    рекомендуемая версия MATLAB R2024a

    sudo apt install openjdk-17-jdk
    sudo apt install maven
    sudo apt install gcc-11 g++-11
    sudo apt install make
    sudo apt install cmake
    sudo apt install libtool

        Установка libzeromq и czmq
    libzeromq:
    sudo apt-get install libzmq3-dev
    czmq:
    git clone https://github.com/zeromq/czmq.git
    cd czmq
    ./autogen.sh && ./configure
    sudo make install
    sudo ldconfig
    cd ..


        Установка jeromq
    git clone -b v0.6.0 https://github.com/zeromq/jeromq.git
    cd jeromq
    JAVA_HOME="/usr/lib/jvm/java-17-openjdk-amd64"
    echo $JAVA_HOME
    mvn clean package
    результат: target/jeromq-0.6.0.jar
    cd ..

        Установка srsran_4g
    Необходимые копоненты для srsran: https://docs.srsran.com/en/latest
    git clone https://github.com/srsRAN/srsRAN_4G.git
    cd srsRAN_4G
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Debug ..
    make -j$(nproc)
    sudo make install
    sudo srsran_install_configs.sh user
    sudo srsran_install_configs.sh service
    cd ..

### Установка proxy server

    git clone https://github.com/Kandrw/Mobile_system_NG
    Скопировать полный путь до jeromq-0.6.0.jar
    cd Mobile_system_NG/matlab
    nano subscribe.m (или в самом matlab)
    
    Заменить путь в javaaddpath в скрипте Mobile_system_NG/matlab/subscribe.m 
    
    cd Mobile_system_NG
    mkdir build
    cd build
    cmake ..
    make
    cd ../..


### Запуск

    Каждую программу необходимо запускать в отдельном терминале 

    запустить скрипт Mobile_system_NG/matlab/subscribe.m 
        cd Mobile_system_NG/matlab
        Открыть matlab, запустить скрипт subscribe.m
    
    запустить proxy server
        cd Mobile_system_NG/build
        ./tcp_proxy 2000 2005 2006 2001 2111
    
    Запуск srsran
        cd srsRAN_4G

        sudo build/srsepc/src/srsepc

        sudo build/srsenb/src/srsenb --rf.device_name=zmq --rf.device_args="fail_on_disconnect=true,tx_port=tcp://*:2000,rx_port=tcp://localhost:2001,id=enb,base_srate=23.04e6"

        sudo build/srsue/src/srsue --rf.device_name=zmq --rf.device_args="tx_port=tcp://*:2005,rx_port=tcp://localhost:2006,id=ue,base_srate=23.04e6" --gw.netns=ue1

    Итог
        вывод графиков в matlab

    При остановке одного из компонентов необходимо перезапустить все остальные
    При перезаупуске удостовериться что все порты свободны
        netstat -tulnp 














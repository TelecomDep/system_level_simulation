#include <iostream>
#include <mutex>
#include <zmq.h>
#include <thread>
#include <cstring>
#include <cmath>
#include <unistd.h>

#include <vector>
#include <random>
#include <complex>
#include <algorithm>
#include <time.h>

#include "includes/Broker.hpp"
#include "includes/subfunc.hpp"


void my_handler(int s){
    printf("Caught signal %d\n",s);
    
    exit(1); 

}

int main(){

    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
 
    sigaction(SIGINT, &sigIntHandler, NULL);

    std::string config_file_path = "../configs/broker.json";

    std::vector<Equipment> ues;
    std::vector<Equipment> gnbs;

    // ues.push_back(Equipment(2100, 2101, 1, 1));
    // ues.push_back(Equipment(2200, 2201, 1, 1));
    ues.push_back(Equipment(2110, 2111, 1, 1));
    ues.push_back(Equipment(2120, 2121, 2, 1));
    // ues.push_back(Equipment(2130, 2131, 3, 1));

    gnbs.push_back(Equipment(2001, 2000, 255, 0));
    Broker broker = Broker(config_file_path, ues, gnbs);
    broker.enable_matlab = false;

    broker.start_the_proxy();

    return 0;
}
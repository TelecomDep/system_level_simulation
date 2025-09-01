#include <iostream>

#include <csignal>

#include "includes/Broker.hpp"

#include "includes/subfunc.hpp"

typedef _Complex float cf_t;

#define BUFFER_MAX 1024 * 1024
#define NSAMPLES2NBYTES(X) (((uint32_t)(X)) * sizeof(cf_t))
#define NBYTES2NSAMPLES(X) ((X) / sizeof(cf_t))
#define ZMQ_MAX_BUFFER_SIZE (NSAMPLES2NBYTES(3072000)) // 10 subframes at 20 MHz
#define NBYTES_PER_ONE_SAMPLE (NSAMPLES2NBYTES(1)) // 1 sample

int main(int argc, char *argv[]){

    //CTRL+C handler
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
 
    sigaction(SIGINT, &sigIntHandler, NULL);

    std::string path_to_config;

    if(!check_cli_args(argc, argv, path_to_config)){
        return 1;
    }

    broker_args config_args;

    //parsing
    if(broker_config_parser(&config_args, path_to_config) != 0){
        printf("ERROR: Invalid parsing");
        return 1;
    }

    //split strings with ports
    std::vector<int> ue_rx_port = ports_to_int(config_args.ue_rx_port);
    std::vector<int> ue_tx_port = ports_to_int(config_args.ue_tx_port);

    //check errors
    if(config_args.ue_count != ue_rx_port.size() || config_args.ue_count != ue_tx_port.size()){
        printf("\nERROR: Ue count not equal ports count\n");
        return 1;
    }

    if(config_args.ue_count == -1){
        printf("ERROR: Config file does not contain ue_count");
        return 1;
    }

    if(config_args.gnb_rx_port == -1){
        printf("ERROR: Config file does not contain gnb_rx_port");
        return 1;
    }

    if(config_args.gnb_tx_port == -1){
        printf("ERROR: Config file does not contain gnb_tx_port");
        return 1;
    }

    if(config_args.matlab_port == -1){
        printf("ERROR: Config file does not contain matlab_port");
        return 1;
    }

    int num_ues = config_args.ue_count;
    int num_gnbs = 1;

    std::vector<UserEquipment> ues;

    //create ues
    for(int i = 0; i < num_ues; ++i){
        ues.push_back(UserEquipment(ue_rx_port[i], ue_tx_port[i]));
    }

    //create gnb
    std::vector<gNodeB> gnbs;

    gnbs.push_back(gNodeB(config_args.gnb_rx_port, config_args.gnb_tx_port));

    std::cout << "\n========= Start ZMQ =========\n";

    std::cout << "ZMQ_MAX_BUFFER_SIZE = " << ZMQ_MAX_BUFFER_SIZE << std::endl;
    std::cout << "NBYTES_PER_ONE_SAMPLE = " << NBYTES_PER_ONE_SAMPLE << std::endl;
    std::cout << "sizeof(cf_t) = " << sizeof(cf_t) << std::endl;

    //create broker

    Broker broker;

    if(!config_args.matlab_enable)
        broker = Broker(ues, gnbs);
    else
        broker = Broker(ues, gnbs, config_args.matlab_port);

    broker.run();

    return 0;
}  
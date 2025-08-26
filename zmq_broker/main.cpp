#include <iostream>

#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>

#include <fstream>

#include <csignal>

#include "includes/Broker.hpp"

namespace bpo = boost::program_options;

typedef _Complex float cf_t;

#define BUFFER_MAX 1024 * 1024
#define NSAMPLES2NBYTES(X) (((uint32_t)(X)) * sizeof(cf_t))
#define NBYTES2NSAMPLES(X) ((X) / sizeof(cf_t))
#define ZMQ_MAX_BUFFER_SIZE (NSAMPLES2NBYTES(3072000)) // 10 subframes at 20 MHz
#define NBYTES_PER_ONE_SAMPLE (NSAMPLES2NBYTES(1)) // 1 sample

#define DEFAULT_PATH_TO_CONFIG "../configs/zmq_broker.conf"

bool        use_standard_lte_rates = false;
std::string scs_khz, ssb_scs_khz;
std::string cfr_mode;

struct broker_args{
    int gnb_rx_port;
    int gnb_tx_port;

    int ue_count;

    bool matlab_enable;
    int matlab_port; 

    std::vector<std::string> ue_rx_port;
    std::vector<std::string> ue_tx_port;
};

int broker_config_parser(broker_args* args, std::string config_file){
    
    bpo::options_description common("Configuration options");

    common.add_options()

    //gnb
    ("gnb.rx_port", bpo::value<int>(&args->gnb_rx_port)->default_value(-1), "gnb rx port")
    ("gnb.tx_port", bpo::value<int>(&args->gnb_tx_port)->default_value(-1), "gnb tx port")

    //ue
    ("ue.rx_ports", bpo::value<std::vector<std::string>>(&args->ue_rx_port)->multitoken(), "ue rx port")
    ("ue.tx_ports", bpo::value<std::vector<std::string>>(&args->ue_tx_port)->multitoken(), "ue tx port")
    ("ue.count", bpo::value<int>(&args->ue_count)->default_value(-1), "ue_count")

    //matlab
    ("matlab.port", bpo::value<int>(&args->matlab_port)->default_value(-1), "matlab port")
    ("matlab.enable", bpo::value<bool>(&args->matlab_enable)->default_value(false), "matlab using");

    std::ifstream conf(config_file.c_str(), std::ios::in);

    if (!conf) {
        std::cout << "\nCannot open config file: " << config_file << std::endl;
        return 1;
    }

    bpo::variables_map vm;

    try {
        bpo::store(bpo::parse_config_file(conf, common), vm);
        bpo::notify(vm);
    } catch (const bpo::error& e) {
        std::cout << "\nError parsing config file: " << e.what() << std::endl;
        return 2;
    }

    return 0;

}

void my_handler(int s){
    printf("\n===== End ZMQ =====\n");
    
    exit(1); 
}

std::vector<int> ports_to_int(std::vector<std::string> ports){
    std::vector<int> result;

    char sep[] = " ";
    char *token;

    token = strtok(ports[0].data(), sep);

    while (token != NULL) {
         try{
            result.push_back(std::stoi(token));
        }catch (const std::invalid_argument& e) {
            printf("\nERROR: Cannot convert port number to int\n");
            return {};
        }

        token = strtok(NULL, sep); 
    }

    return result;
}


int main(int argc, char *argv[]){

    std::string path_to_config;

    if(argc > 2){
        printf("Invalid arguments");
        return 1;
    }

    if(argc == 2){
        path_to_config = argv[1];
    }

    if(argc == 1){
        path_to_config = DEFAULT_PATH_TO_CONFIG;
    }

    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
 
    sigaction(SIGINT, &sigIntHandler, NULL);

    broker_args args;

    //parsing
    if(broker_config_parser(&args, path_to_config) != 0){
        printf("ERROR: Invalid parsing");
        return 1;
    }

    //split strings with ports
    std::vector<int> ue_rx_port = ports_to_int(args.ue_rx_port);
    std::vector<int> ue_tx_port = ports_to_int(args.ue_tx_port);

    //check errors
    if(args.ue_count != ue_rx_port.size() || args.ue_count != ue_tx_port.size()){
        printf("\nERROR: Ue count not equal ports count\n");
        return 1;
    }

    if(args.ue_count == -1){
        printf("ERROR: Config file does not contain ue_count");
        return 1;
    }

    if(args.gnb_rx_port == -1){
        printf("ERROR: Config file does not contain gnb_rx_port");
        return 1;
    }

    if(args.gnb_tx_port == -1){
        printf("ERROR: Config file does not contain gnb_tx_port");
        return 1;
    }

    if(args.matlab_port == -1){
        printf("ERROR: Config file does not contain matlab_port");
        return 1;
    }

    int num_ues = args.ue_count;
    int num_gnbs = 1;

    std::vector<UserEquipment> ues;

    //create ues
    for(int i = 0; i < num_ues; ++i){
        ues.push_back(UserEquipment(ue_rx_port[i], ue_tx_port[i]));
    }

    //create gnb
    std::vector<gNodeB> gnbs;

    gnbs.push_back(gNodeB(args.gnb_rx_port, args.gnb_tx_port));

    std::cout << "\n========= Start ZMQ =========\n";

    std::cout << "ZMQ_MAX_BUFFER_SIZE = " << ZMQ_MAX_BUFFER_SIZE << std::endl;
    std::cout << "NBYTES_PER_ONE_SAMPLE = " << NBYTES_PER_ONE_SAMPLE << std::endl;
    std::cout << "sizeof(cf_t) = " << sizeof(cf_t) << std::endl;

    //create broker

    Broker broker;

    if(!args.matlab_enable)
        broker = Broker(ues, gnbs);
    else
        broker = Broker(ues, gnbs, args.matlab_port);

    broker.run();

    return 0;
}  
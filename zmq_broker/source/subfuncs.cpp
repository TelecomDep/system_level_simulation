#include "../includes/subfunc.hpp"

namespace bpo = boost::program_options;

bool        use_standard_lte_rates = false;
std::string scs_khz, ssb_scs_khz;
std::string cfr_mode;

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
    ("matlab_prototype.port", bpo::value<int>(&args->matlab_port)->default_value(-1), "matlab port")
    ("matlab_prototype.enable", bpo::value<bool>(&args->matlab_enable)->default_value(false), "matlab using");

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

bool check_cli_args(int argc, char* argv[], std::string& path_to_config){

    if(argc > 2){
        printf("Invalid arguments");
        return false;
    }

    if(argc == 2){
        path_to_config = argv[1];
        return true;
    }

    if(argc == 1){
        path_to_config = DEFAULT_PATH_TO_CONFIG;
        return true;
    }

    return false;
}
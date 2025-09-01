#pragma once

#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>

#include <vector>
#include <iostream>

#include <fstream>

#define DEFAULT_PATH_TO_CONFIG "../configs/zmq_broker.conf"

struct broker_args{
    int gnb_rx_port;
    int gnb_tx_port;

    int ue_count;

    bool matlab_enable;
    int matlab_port; 

    std::vector<std::string> ue_rx_port;
    std::vector<std::string> ue_tx_port;
};

int broker_config_parser(broker_args* args, std::string config_file);

void my_handler(int s);

std::vector<int> ports_to_int(std::vector<std::string> ports);

bool check_cli_args(int argc, char* argv[], std::string path_to_config);
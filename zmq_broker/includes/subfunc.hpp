#pragma once

#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>

#include <vector>
#include <iostream>

#include <fstream>

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


std::vector<int> ports_to_int(std::vector<std::string> ports);
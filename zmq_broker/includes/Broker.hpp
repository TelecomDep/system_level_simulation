#pragma once
#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <zmq.h>
#include <chrono>

#include "gNB.hpp"
#include "UE.hpp"

class Broker{
    private:
    
        std::vector<UserEquipment> ues;
        std::vector<gNodeB> gnbs;

        int matlab_port;

        static int broker_count;

        //data transmission logic and subfunction

        std::vector<std::complex<float>> byte_to_complex(const std::vector<uint8_t>& buffer);
        int send_to_matlab(void *socket_to_matlab, std::vector<uint8_t> &data, int data_size, char *id_data, size_t id_size);
        int receive_from_matlab(void *socket_to_matlab, std::vector<uint8_t> &data, int target_rcv_data);

        template <typename T>
        std::vector<uint8_t> to_byte(std::vector<T> data);

        std::vector<uint8_t> concatenate_tx_samples();
        std::vector<std::vector<std::complex<float>>> deconcatenate_all_samples(std::vector<uint8_t> &all_samples, std::vector<int> packet_sizes);

    public:

        Broker(std::vector<UserEquipment>& _ues, std::vector<gNodeB>& _gnbs, int _matlab_port = -1);
        Broker();

        //getters
        std::vector<UserEquipment> get_list_of_ues() const;
        std::vector<gNodeB> get_list_of_gnbs() const;

        //samples sizes from gnbs and ues
        std::vector<int> get_tx_samples_sizes() const;
        std::vector<int> get_rx_samples_sizes() const;

        //samples from ues and gnbs
        std::vector<std::vector<std::complex<float>>> get_all_tx_samples() const;

        //setters
        void set_list_of_ues(std::vector<UserEquipment> _ues);
        void set_list_of_gnbs(std::vector<gNodeB> _gnbs);

        void run();
};
#pragma once
#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <complex>
#include <vector>
#include <zmq.h>

#include "gNB.hpp"
#include "UE.hpp"
#include "Equipment.hpp"

#define BUFFER_MAX 1024 * 1024


typedef _Complex float cf_t;
#define NSAMPLES2NBYTES(X) (((uint32_t)(X)) * sizeof(cf_t))
#define NBYTES2NSAMPLES(X) ((X) / sizeof(cf_t))
#define ZMQ_MAX_BUFFER_SIZE (NSAMPLES2NBYTES(3072000)) // 10 subframes at 20 MHz
#define NBYTES_PER_ONE_SAMPLE (NSAMPLES2NBYTES(1)) // 1 sample


class Broker{
    public:
    
        std::vector<Equipment> ues;
        std::vector<Equipment> gnbs;

        bool is_matlab_connected;
        int matlab_port;

        void *zmq_context;
        void *matlab_req_socket;

        bool is_running = true;
        int broker_acc_count;

        int buff_size = 100000;
        int nbytes_form_gnb = 0;
        int broker_working_counter = 0;

        std::vector<std::complex<float>> concatenate_to_gnb_samples;
        std::vector<std::complex<float>> matlab_samples;

        //data transmission logic and subfunction

        std::vector<std::complex<float>> byte_to_complex(const std::vector<uint8_t>& buffer);
        int send_to_matlab(void *socket_to_matlab, std::vector<uint8_t> &data, int data_size, char *id_data, size_t id_size);
        int receive_from_matlab(void *socket_to_matlab, std::vector<uint8_t> &data, int target_rcv_data);

        template <typename T>
        std::vector<uint8_t> to_byte(std::vector<T> data);
        std::vector<std::vector<std::complex<float>>> deconcatenate_all_samples(std::vector<uint8_t> &all_samples, std::vector<int> packet_sizes);

    private:
        void initialize_zmq_sockets();

        // Connection requests and accepts
        bool recv_conn_accepts();
        bool send_conn_accepts();

        // Samples transmission
        bool recv_samples_from_gNb();
        bool send_samples_to_gnb();
        bool send_samples_to_all_ues();
        bool recv_samples_from_ues();

        // Matlab
        void send_recv_samples_from_gNb_to_matlab();
        void send_recv_samples_from_Ues_to_matlab();

        void run_the_world();

    public:
        Broker(std::vector<UserEquipment>& _ues, std::vector<gNodeB>& _gnbs, int _matlab_port = -1);
        Broker(std::string &config_file, std::vector<Equipment>& _ues, std::vector<Equipment>& _gnbs);
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
        void start_the_proxy();
};
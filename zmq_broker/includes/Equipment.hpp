#pragma once

#include <complex>
#include <vector>

class Equipment {
    public:
        int id;
        int type;
        int rx_port;
        int tx_port;
        int N = 100000;
        
        std::vector<std::complex<float>> samples_rx;
        std::vector<std::complex<float>> samples_to_transmit;

        // client on our side  - server on the srsRAN side
        void *req_for_srsran_tx_socket = nullptr; 

        // server on our side - client on the srsRAN side
        void *rep_for_srsran_rx_socket = nullptr;

        int is_recv_conn_acc_from_rx = 0;
        int is_send_conn_req_to_tx = 0;
        char buffer_recv_conn_req[10];
        char buffer_send_conn_req[10];
        int curr_recv_from_tx_pack_size = 0;

    public:
        Equipment(int port_rx, int port_tx, int id, int type);

        void initialize_sockets(void *zmq_context);

        void recv_conn_accept();
        void send_conn_accept();

        int recv_samples_from_tx(int buff_size);
        void send_samples_to_rx(std::vector<std::complex<float>>& samples, int buff_size);

        int get_nbytes_recv_from_tx();

    public:
        int is_ready_to_send();
        bool is_ready_to_recv();

        // getters
        int get_rx_port() const;
        int get_tx_port() const;
        std::vector<std::complex<float>>& get_samples_rx();
        std::vector<std::complex<float>>& get_samples_tx();

        //setters
        void set_rx_port(int port);
        void set_tx_port(int port);
        void set_samples_rx(const std::vector<std::complex<float>>& samples, const int size);
        void set_samples_tx(const std::vector<std::complex<float>>& samples, const int size);

        //samples clear
        void clear_samples_rx();
        void clear_samples_tx();
};
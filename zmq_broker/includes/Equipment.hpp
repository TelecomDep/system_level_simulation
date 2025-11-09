#pragma once

#include <complex>
#include <vector>

#define BUFFER_MAX 1024 * 1024
#define NSAMPLES2NBYTES(X) (((uint32_t)(X)) * sizeof(cf_t))
#define NBYTES2NSAMPLES(X) ((X) / sizeof(cf_t))
#define ZMQ_MAX_BUFFER_SIZE (NSAMPLES2NBYTES(3072000)) // 10 subframes at 20 MHz
#define NBYTES_PER_ONE_SAMPLE (NSAMPLES2NBYTES(1)) // 1 sample

class Equipment {
    public:
        int id;
        int type;
        int rx_port;
        int tx_port;
        int N = 100000;

        bool rx_ready = false;
        bool tx_samples_ready = false;
        uint8_t dummy = 0;
        int dummy_size = 0;

        std::vector<std::complex<float>> samples_rx;
        std::vector<std::complex<float>> samples_to_transmit;
        int ready_to_tx_n_bytes = 0;

        // client on our side  - server on the srsRAN side
        void *req_for_srsran_tx_socket = nullptr; 

        // server on our side - client on the srsRAN side
        void *rep_for_srsran_rx_socket = nullptr;
        int sent_to_gnb = 0;
        int is_recv_conn_acc_from_rx = 0;
        int is_send_conn_req_to_tx = 0;
        char buffer_recv_conn_req[10];
        char buffer_send_conn_req[10];
        int curr_recv_from_tx_pack_size = 0;
        bool is_active = true;

    public:
        Equipment(int port_rx, int port_tx, int id, int type);

        // Async working
        void initialize_sockets(void *zmq_context);
        void rep_recv_conn_request_from_req();
        void send_req_to_get_samples_from_rep(uint8_t opposite_dummy, int opposite_size);
        void send_samples_to_req_rx(std::vector<std::complex<float>>& samples, int nbytes);

        bool is_rx_ready();
        bool is_tx_samples_ready();
        int get_ready_to_tx_bytes();

        // OLD
        void recv_conn_accept();
        void send_conn_accept();

        int recv_samples_from_tx(int buff_size);
        void send_samples_to_rx(std::vector<std::complex<float>>& samples, int buff_size);

        int get_nbytes_recv_from_tx();

        void divide_samples_by_value(float pl);
        void activate();

        int getId();

    public:
        int is_ready_to_send();
        bool is_ready_to_recv();
        int get_recv_nbytes();

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
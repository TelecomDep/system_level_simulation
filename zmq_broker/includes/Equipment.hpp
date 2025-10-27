#pragma once

#include <complex>
#include <vector>

class Equipment {
    private:
        int id;
        int type;
        int rx_port;
        int tx_port;
        std::vector<std::complex<float>> samples_rx;
        std::vector<std::complex<float>> samples_tx;

        // client on our side  - server on the srsRAN side
        void *req_for_srsran_tx_socket; 

        // server on our side - client on the srsRAN side
        void *rep_for_srsran_rx_socket; 

    public:
        Equipment(int port_rx, int port_tx, int id, int type);

        void initialize_sockets(void *zmq_context);

        //getters
        int get_rx_port() const;
        int get_tx_port() const;
        const std::vector<std::complex<float>>& get_samples_rx() const;
        const std::vector<std::complex<float>>& get_samples_tx() const;

        //setters
        void set_rx_port(int port);
        void set_tx_port(int port);
        void set_samples_rx(const std::vector<std::complex<float>>& samples, const int size);
        void set_samples_tx(const std::vector<std::complex<float>>& samples, const int size);

        //samples clear
        void clear_samples_rx();
        void clear_samples_tx();
};
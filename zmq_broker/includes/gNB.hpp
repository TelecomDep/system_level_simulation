#pragma once

#include <complex>
#include <vector>

class gNodeB {
    private:
        int rx_port;
        int tx_port;
        std::vector<std::complex<float>> samples_rx;
        std::vector<std::complex<float>> samples_tx;

    public:
        gNodeB(int port_rx, int port_tx);

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

        //sample size
        size_t size_samples_rx() const;
        size_t size_samples_tx() const;

        //sample add
        void add_sample_rx(std::complex<float> sample);
        void add_sample_tx(std::complex<float> sample);

};

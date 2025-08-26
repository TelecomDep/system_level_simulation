#include <iostream>
#include "../includes/gNB.hpp"

//class init
gNodeB::gNodeB(int port_rx, int port_tx): 
    rx_port(port_rx), tx_port(port_tx){
    samples_rx = std::vector<std::complex<float>>();
    samples_tx = std::vector<std::complex<float>>();
}

//getters
int gNodeB::get_rx_port() const {
    return rx_port;
}

int gNodeB::get_tx_port() const {
    return tx_port;
}

const std::vector<std::complex<float>>& gNodeB::get_samples_rx() const{
    return samples_rx;
}

const std::vector<std::complex<float>>& gNodeB::get_samples_tx() const{
    return samples_tx;
}

//setters
void gNodeB::set_rx_port(int port){
    rx_port = port;
}

void gNodeB::set_tx_port(int port){
    tx_port = port;
}

void gNodeB::set_samples_rx(const std::vector<std::complex<float>>& samples, const int size){
    samples_rx.resize(size);
    samples_rx.assign(samples.begin(), samples.begin() + size);
}

void gNodeB::set_samples_tx(const std::vector<std::complex<float>>& samples, const int size){
    samples_rx.resize(size);
    samples_tx.assign(samples.begin(), samples.begin() + size);
}

//sample clear
void gNodeB::clear_samples_rx(){
    samples_rx.clear();
}

void gNodeB::clear_samples_tx(){
    samples_tx.clear();
}

//sample size
size_t gNodeB::size_samples_rx() const{
    return samples_rx.size();
}

size_t gNodeB::size_samples_tx() const{
    return samples_tx.size();
}

//sample add
void gNodeB::add_sample_rx(std::complex<float> sample){
    samples_rx.push_back(sample);
}

void gNodeB::add_sample_tx(std::complex<float> sample){
    samples_tx.push_back(sample);
}
#include <iostream>
#include "../includes/UE.hpp"

//class init
UserEquipment::UserEquipment(int port_rx, int port_tx){
    rx_port = port_rx;
    tx_port = port_tx;
    samples_rx = std::vector<std::complex<float>>();
    samples_tx = std::vector<std::complex<float>>();
}

//getters
int UserEquipment::get_rx_port() const {
    return rx_port;
}

int UserEquipment::get_tx_port() const {
    return tx_port;
}

const std::vector<std::complex<float>>& UserEquipment::get_samples_rx() const{
    return samples_rx;
}

const std::vector<std::complex<float>>& UserEquipment::get_samples_tx() const{
    return samples_tx;
}

//setters
void UserEquipment::set_rx_port(int port){
    rx_port = port;
}

void UserEquipment::set_tx_port(int port){
    tx_port = port;
}

void UserEquipment::set_samples_rx(const std::vector<std::complex<float>>& samples, const int size){
    samples_rx.resize(size);
    samples_rx.assign(samples.begin(), samples.begin() + size);
}

void UserEquipment::set_samples_tx(const std::vector<std::complex<float>>& samples, const int size){
    samples_rx.resize(size);
    samples_tx.assign(samples.begin(), samples.begin() + size);
}

//sample clear
void UserEquipment::clear_samples_rx(){
    samples_rx.clear();
}

void UserEquipment::clear_samples_tx(){
    samples_tx.clear();
}

//sample size
size_t UserEquipment::size_samples_rx() const{
    return samples_rx.size();
}

size_t UserEquipment::size_samples_tx() const{
    return samples_tx.size();
}

//sample add
void UserEquipment::add_sample_rx(std::complex<float> sample){
    samples_rx.push_back(sample);
}

void UserEquipment::add_sample_tx(std::complex<float> sample){
    samples_tx.push_back(sample);
}

void UserEquipment::show_info(){
    printf("RX PORT: %d\nTX PORT: %d\n", rx_port, tx_port);

    printf("RX SAMPLES: \n");

    for(std::complex<float>& el : samples_rx){
        printf("(%f, %f), ", el.real(), el.imag());
    }

    printf("\n");

    printf("TX SAMPLES: \n");

    for(std::complex<float>& el : samples_tx){
        printf("(%f, %f), ", el.real(), el.imag());
    }

    printf("\n");

}
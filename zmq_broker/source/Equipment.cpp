#include <iostream>
#include <zmq.h>
#include "../includes/Equipment.hpp"

//class init
Equipment::Equipment(int port_rx, int port_tx, int _id, int _type){
    id = _id;
    type = _type;
    rx_port = port_rx;
    tx_port = port_tx;
    samples_rx = std::vector<std::complex<float>>();
    samples_tx = std::vector<std::complex<float>>();
}

void Equipment::initialize_sockets(void *zmq_context)
{
    std::string addr_port_tx = "tcp://localhost:" + std::to_string(tx_port);
    std::string addr_port_rx = "tcp://*:" + std::to_string(rx_port);

    printf("|-------------------------------------------|\n");
    req_for_srsran_tx_socket = zmq_socket(zmq_context, ZMQ_REQ);
    int ret = zmq_connect(req_for_srsran_tx_socket, addr_port_tx.c_str());
    if(ret < 0){
        printf("ret = %d\n",ret);
        exit(1);
    } else {
        printf("CONNECTED id[%d] type[%d], ret[%d]\n", id, type, ret);
    }
    

    rep_for_srsran_rx_socket = zmq_socket(zmq_context, ZMQ_REP);
    if(zmq_bind(rep_for_srsran_rx_socket, addr_port_rx.c_str())) {
        printf("BIND FAILED id[%d] type[%d] ret[%d]\n", id, type, ret);
        exit(1);
    } else {
        printf("Bind Success id[%d] type[%d] ret[%d]\n", id, type, ret);
    }
}

//getters
int Equipment::get_rx_port() const {
    return rx_port;
}

int Equipment::get_tx_port() const {
    return tx_port;
}

const std::vector<std::complex<float>>& Equipment::get_samples_rx() const{
    return samples_rx;
}

const std::vector<std::complex<float>>& Equipment::get_samples_tx() const{
    return samples_tx;
}

//setters
void Equipment::set_rx_port(int port){
    rx_port = port;
}

void Equipment::set_tx_port(int port){
    tx_port = port;
}

void Equipment::set_samples_rx(const std::vector<std::complex<float>>& samples, const int size){
    samples_rx.resize(size);
    samples_rx.assign(samples.begin(), samples.begin() + size);
}

void Equipment::set_samples_tx(const std::vector<std::complex<float>>& samples, const int size){
    samples_rx.resize(size);
    samples_tx.assign(samples.begin(), samples.begin() + size);
}

//sample clear
void Equipment::clear_samples_rx(){
    samples_rx.clear();
}

void Equipment::clear_samples_tx(){
    samples_tx.clear();
}
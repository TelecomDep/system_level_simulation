#include <iostream>
#include <cstring>

#include <zmq.h>
#include "../includes/Equipment.hpp"

//class init
Equipment::Equipment(int port_rx, int port_tx, int _id, int _type){
    id = _id;
    type = _type;
    rx_port = port_rx;
    tx_port = port_tx;
    samples_rx = std::vector<std::complex<float>>(N);
    samples_to_transmit = std::vector<std::complex<float>>(N);
}

void Equipment::initialize_sockets(void *zmq_context)
{
    std::string addr_port_tx = "tcp://localhost:" + std::to_string(tx_port);
    std::string addr_port_rx = "tcp://*:" + std::to_string(rx_port);

    printf("|-------------------------------------------|\n");
    req_for_srsran_tx_socket = zmq_socket(zmq_context, ZMQ_REQ);
    if (req_for_srsran_tx_socket == nullptr){
        printf("NULL PTR Socket\n");
        exit(1);
    }
    int timeout = 25000;

    zmq_setsockopt(req_for_srsran_tx_socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

    int ret = zmq_connect(req_for_srsran_tx_socket, addr_port_tx.c_str());
    if(ret < 0){
        printf("ret = %d\n",ret);
        exit(1);
    } else {
        printf("CONNECTED id[%d] type[%d], ret[%d]\n", id, type, ret);
    }
    

    rep_for_srsran_rx_socket = zmq_socket(zmq_context, ZMQ_REP);
    if (rep_for_srsran_rx_socket == nullptr){
        printf("NULL PTR Socket\n");
        exit(1);
    }
    if(zmq_bind(rep_for_srsran_rx_socket, addr_port_rx.c_str())) {
        printf("BIND FAILED id[%d] type[%d] ret[%d]\n", id, type, ret);
        exit(1);
    } else {
        printf("Bind Success id[%d] type[%d] ret[%d]\n", id, type, ret);
    }
}

void Equipment::recv_conn_accept()
{
    memset(buffer_recv_conn_req, 0, sizeof(buffer_recv_conn_req));
    if(rep_for_srsran_rx_socket != nullptr){
        int size = zmq_recv(rep_for_srsran_rx_socket, buffer_recv_conn_req, sizeof(buffer_recv_conn_req), 0);
        if(size == -1){
            printf("-->> Equipment (id[%d] type[%d]) did not recieved connection Request from RX(client) port[%d]\n", id, type, rx_port);
            is_recv_conn_acc_from_rx = 0;
        } else{
            is_recv_conn_acc_from_rx = 1;
            curr_recv_from_tx_pack_size = size;
            printf("Equipment (id[%d] type[%d]) received [buffer_recv_conn_req] RX(client) port[%d] size [%d]\n", id, type, rx_port, size);
        }
    } else {
        std::cout << "rep_for_srsran_rx_socket = nullptr" << std::endl;
    }
}

void Equipment::send_conn_accept()
{
    //memset(buffer_send_conn_req, 0, sizeof(buffer_send_conn_req));
    if(rep_for_srsran_rx_socket != nullptr){
        int send = zmq_send(req_for_srsran_tx_socket, buffer_recv_conn_req, sizeof(buffer_recv_conn_req[0]), 0);
        if(send == -1){
            printf("Equipment (id[%d] type[%d]) did not send connection Request to TX(server) port[%d]\n", id, type,tx_port);
            is_send_conn_req_to_tx = 0;
        } else{
            is_send_conn_req_to_tx = 1;
            printf("Equipment (id[%d] type[%d]) send [buffer_send_conn_req] to TX(server) port[%d] = %d\n", id, type, tx_port, send);
        }
    } else{
        std::cout << "req_for_srsran_tx_socket = nullptr" << std::endl;
    }
}

int Equipment::recv_samples_from_tx(int buff_size)
{
    int nbytes = samples_to_transmit.size() * sizeof(std::complex<float>);
    std::fill(samples_to_transmit.begin(), samples_to_transmit.end(), 0);

    if (req_for_srsran_tx_socket != nullptr)
    {
        int size = zmq_recv(req_for_srsran_tx_socket,  (void*)samples_to_transmit.data(), nbytes, 0);
        if (size != -1)
        {
            printf("Broker received from server id[%d] type[%d] =  packet size [%d]\n",id, type, size);
            curr_recv_from_tx_pack_size = size;
        } else {
            printf("-->> Error receiving samples\n", size);
        }
    }
    else
    {
        std::cout << "req_for_srsran_tx_socket = nullptr" << std::endl;
    }
    return curr_recv_from_tx_pack_size;
}

void Equipment::send_samples_to_rx(std::vector<std::complex<float>>& samples, int buff_size)
{
    //int nbytes = buff_size * sizeof(std::complex<float>)/8;
    int send = zmq_send(rep_for_srsran_rx_socket, (void*)samples.data(), buff_size, 0);
    if(send != -1){
        printf("Send samples client socket: send data[%d], nBytes[%d]\n", send, buff_size);
    } else {
        printf("-->> Error receiving samples\n", send);
    }
}

int Equipment::is_ready_to_send()
{
    return is_recv_conn_acc_from_rx;
}

bool Equipment::is_ready_to_recv()
{
    return is_send_conn_req_to_tx;
}

int Equipment::get_nbytes_recv_from_tx()
{
    return curr_recv_from_tx_pack_size;
}

//getters
int Equipment::get_rx_port() const {
    return rx_port;
}

int Equipment::get_tx_port() const {
    return tx_port;
}

std::vector<std::complex<float>>& Equipment::get_samples_rx(){
    return samples_rx;
}

std::vector<std::complex<float>>& Equipment::get_samples_tx(){
    return samples_to_transmit;
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
    samples_to_transmit.assign(samples.begin(), samples.begin() + size);
}

//sample clear
void Equipment::clear_samples_rx(){
    samples_rx.clear();
}

void Equipment::clear_samples_tx(){
    samples_to_transmit.clear();
}
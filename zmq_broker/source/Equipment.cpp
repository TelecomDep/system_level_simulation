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
    // int timeout = 25000;

    // zmq_setsockopt(req_for_srsran_tx_socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

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

void Equipment::rep_recv_conn_request_from_req()
{
    if(!this->rx_ready){
        int size = zmq_recv(rep_for_srsran_rx_socket, &dummy, sizeof(dummy), ZMQ_DONTWAIT);
        if(size == -1){
            printf("!!! Equipment (id[%d] type[%d]) did not recieved connection Request from RX(client) port[%d]\n", id, type, rx_port);
            this->dummy_size = size;
        }
        else
        {
            this->rx_ready = true;
            this->dummy_size = size;
            printf("broker received [buffer_acc] form RX = %d dummy = %d\n", size, dummy);
        }
    }
}

void Equipment::send_req_to_get_samples_from_rep(uint8_t opposite_dummy, int opposite_size)
{

    if(!this->tx_samples_ready){
        int send = zmq_send(req_for_srsran_tx_socket, &opposite_dummy, opposite_size, 0);
        printf("req_socket_from_gnb_tx [send] %d id[%d] type[%d]\n",send, id, type);
    }
    
    //if(send >= 0){
        //std::fill(samples_to_transmit.begin(), samples_to_transmit.end(), 0);
        int nbytes = samples_to_transmit.size() * sizeof(std::complex<float>);
        int size = zmq_recv(req_for_srsran_tx_socket,  (void*)samples_to_transmit.data(), nbytes, ZMQ_DONTWAIT);
        if (size != -1)
        {
            this->ready_to_tx_n_bytes = size;
            this->tx_samples_ready = true;
            printf("Broker received from server id[%d] type[%d] =  packet size [%d]\n",id, type, size);
        } else {
            this->tx_samples_ready = false;
            printf("!!gnb_tx_samples_ready = false;  id[%d] type[%d] =  packet size [%d]\n",id, type, size);
        }
    //}
    
    
}

void Equipment::send_samples_to_req_rx(std::vector<std::complex<float>>& samples, int nbytes)
{
    if(this->rx_ready){
        int send = zmq_send(rep_for_srsran_rx_socket, (void*)samples.data(), nbytes, 0);
        printf("rep_socket_for_ue_1_rx [send data] = %d  id[%d] type[%d]\n",send, id, type);
        this->rx_ready = false;
    }
}

bool Equipment::is_rx_ready()
{
    return this->rx_ready;
}

bool Equipment::is_tx_samples_ready()
{
    return this->tx_samples_ready;
}

void Equipment::recv_conn_accept()
{
    if(rep_for_srsran_rx_socket != nullptr){
        int size = zmq_recv(rep_for_srsran_rx_socket, &dummy, sizeof(dummy), NULL);
        if(size == -1){
            printf("!!! Equipment (id[%d] type[%d]) did not recieved connection Request from RX(client) port[%d]\n", id, type, rx_port);
            is_recv_conn_acc_from_rx = 0;
        } else{
            is_recv_conn_acc_from_rx = 1;
            curr_recv_from_tx_pack_size = size;
            printf("Equipment (id[%d] type[%d]) received [buffer_recv_conn_req] RX(client) port[%d] size [%d] dummy [%d]\n", id, type, rx_port, size, dummy);
        }
    } else {
        std::cout << "rep_for_srsran_rx_socket = nullptr" << std::endl;
    }
}

void Equipment::send_conn_accept(uint8_t opposite_dummy, int opposite_size)
{
    //memset(buffer_send_conn_req, 0, sizeof(buffer_send_conn_req));
    if(rep_for_srsran_rx_socket != nullptr && is_active){
        int send = zmq_send(req_for_srsran_tx_socket, &opposite_dummy, opposite_size, 0);
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
    //std::fill(samples_to_transmit.begin(), samples_to_transmit.end(), 0);

    if (req_for_srsran_tx_socket != nullptr && is_active)
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

int Equipment::get_ready_to_tx_bytes()
{
    return this->ready_to_tx_n_bytes;
}

int Equipment::get_recv_nbytes()
{
    return this->curr_recv_from_tx_pack_size;
}

void Equipment::send_samples_to_rx(std::vector<std::complex<float>>& samples, int buff_size)
{
    if(rep_for_srsran_rx_socket != nullptr && is_active){
        int send = zmq_send(rep_for_srsran_rx_socket, (void*)samples.data(), buff_size, 0);
        if(send != -1){
            printf("Send samples client socket: send data[%d], nBytes[%d]\n", send, buff_size);
        } else {
            printf("-->> Error receiving samples\n", send);
        }
    }
}

void Equipment::divide_samples_by_value(float pl)
{
    if(is_active)
    {
        for (int i = 0; i < samples_to_transmit.size(); i++){
            std::complex<float> val_1;
            val_1 = std::complex<float>(samples_to_transmit[i].real()/pl, samples_to_transmit[i].imag()/pl);
            samples_to_transmit[i] = val_1;
        }
    }

}

void Equipment::activate()
{
    is_active = true;
}

int Equipment::getId()
{
    return this->id;
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
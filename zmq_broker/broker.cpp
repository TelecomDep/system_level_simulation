#include <iostream>
#include <mutex>
#include <zmq.h>
#include <thread>
#include <cstring>
#include <cmath>
#include <unistd.h>

#include <vector>
#include <random>
#include <complex>

#define BUFFER_MAX 1024 * 1024


int main(int argc, char *argv[]){

    int ret = 0;

    int port_gnb_tx = 2000;
    int port_gnb_rx = 2001;

    int port_ue_1_tx = 2101;
    int port_ue_1_rx = 2100;

    std::string addr_recv_port_gnb_tx = "tcp://localhost:" + std::to_string(port_gnb_tx);
    std::string addr_recv_port_gnb_rx = "tcp://*:" + std::to_string(port_gnb_rx);

    std::string addr_recv_port_ue_1_tx = "tcp://localhost:" + std::to_string(port_ue_1_tx);
    std::string addr_send_port_ue_1_rx = "tcp://*:" + std::to_string(port_ue_1_rx);

    // Initialize ZMQ
    void *context = zmq_ctx_new ();
    void *req_socket_from_gnb_tx = zmq_socket (context, ZMQ_REQ);
    void *req_socket_from_ue_1_tx = zmq_socket (context, ZMQ_REQ);

    ret = zmq_connect(req_socket_from_gnb_tx, addr_recv_port_gnb_tx.c_str());
    printf("ret = %d\n",ret);
    ret = zmq_connect(req_socket_from_ue_1_tx, addr_recv_port_ue_1_tx.c_str());
    printf("ret = %d\n",ret);

    void *send_socket_for_gnb_rx = zmq_socket(context, ZMQ_REP);
    void *send_socket_for_ue_1_rx = zmq_socket(context, ZMQ_REP);

    if(zmq_bind(send_socket_for_gnb_rx, addr_recv_port_gnb_rx.c_str())) {
        perror("zmq_bind");
        return 1;
    } else {
        printf("zmq_bind [send_socket_for_gnb_rx]- success\n");
    }

    if(zmq_bind(send_socket_for_ue_1_rx, addr_send_port_ue_1_rx.c_str())) {
        perror("zmq_bind");
        return 1;
    } else {
        printf("zmq_bind [send_socket_for_ue_1_rx] - success\n");
    }

    char buffer[BUFFER_MAX];
    char buffer_ul[BUFFER_MAX];
    int size;
    int size_recv;
    int size_from_ue_tx;

    while(1){
        // Handshake
        printf("2\n");
        memset(buffer, 0, sizeof(buffer));
        size = zmq_recv(send_socket_for_ue_1_rx, buffer, sizeof(buffer), 0);
        if(size != -1){
            printf("broker received %d size packet id [%d]\n", size);
        }
        zmq_send(req_socket_from_gnb_tx, buffer, size, 0);

        printf("4\n");
        memset(buffer, 0, sizeof(buffer));
        size = zmq_recv(send_socket_for_gnb_rx, buffer, sizeof(buffer), 0);
        if(size != -1){
            printf("broker received %d size packet id [%d]\n", size);
        }
        zmq_send(req_socket_from_ue_1_tx, buffer, size, 0);




        // Data transmission
        memset(buffer, 0, sizeof(buffer));
        size = zmq_recv(req_socket_from_gnb_tx, buffer, sizeof(buffer), 0);
        if(size != -1){
            printf("broker received %d size packet id [%d]\n", size);
        }
        zmq_send(send_socket_for_ue_1_rx, buffer, size, 0);

        printf("3\n");
        memset(buffer, 0, sizeof(buffer));
        size = zmq_recv(req_socket_from_ue_1_tx, buffer, sizeof(buffer), 0);
        if(size != -1){
            printf("broker received %d size packet id [%d]\n", size);
        }
        zmq_send(send_socket_for_gnb_rx, buffer, size, 0);
    }


    zmq_close (send_socket_for_gnb_rx);
    zmq_close (req_socket_from_ue_1_tx);

    return 0;
}
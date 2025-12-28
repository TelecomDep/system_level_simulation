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
#include <algorithm>
#include <time.h>

#include "includes/Broker.hpp"
#include "includes/subfunc.hpp"


void my_handler(int s){
    printf("Caught signal %d\n",s);
    
    exit(1); 

}

#define BUFFER_MAX 1024 * 1024
#define NSAMPLES2NBYTES(X) (((uint32_t)(X)) * sizeof(cf_t))
#define NBYTES2NSAMPLES(X) ((X) / sizeof(cf_t))
#define ZMQ_MAX_BUFFER_SIZE (NSAMPLES2NBYTES(3072000)) // 10 subframes at 20 MHz
#define NBYTES_PER_ONE_SAMPLE (NSAMPLES2NBYTES(1)) // 1 sample

int main(int argc, char *argv[]){

    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
 
    sigaction(SIGINT, &sigIntHandler, NULL);

    // std::cout << "ZMQ_MAX_BUFFER_SIZE = " << ZMQ_MAX_BUFFER_SIZE << std::endl;
    // std::cout << "NBYTES_PER_ONE_SAMPLE = " << NBYTES_PER_ONE_SAMPLE << std::endl;
    // std::cout << "sizeof(cf_t) = " << sizeof(cf_t) << std::endl;

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


    void *rep_socket_for_gnb_rx = zmq_socket(context, ZMQ_REP);
    void *rep_socket_for_ue_1_rx = zmq_socket(context, ZMQ_REP);

    if(zmq_bind(rep_socket_for_gnb_rx, addr_recv_port_gnb_rx.c_str())) {
        perror("zmq_bind");
        return 1;
    } else {
        printf("zmq_bind [rep_socket_for_gnb_rx]- success\n");
    }

    if(zmq_bind(rep_socket_for_ue_1_rx, addr_send_port_ue_1_rx.c_str())) {
        perror("zmq_bind");
        return 1;
    } else {
        printf("zmq_bind [rep_socket_for_ue_1_rx] - success\n");
    }

    using cf_t = std::complex<float>;
    int N = 100000;
    std::vector<cf_t> buffer_vec(N);
    std::vector<cf_t> buffer_recv_from_ue(N);
    std::vector<cf_t> buffer_vec_to_gnb(N);

    char buffer_acc[10];
    char buffer[BUFFER_MAX];
    int size = 0;
    int size_ue_2 = 0;
    int size_recv = 0;
    int size_from_ue_tx = 0;

    int num_ues = 1;
    int num_gnbs = 1;
    int broker_rcv_accept_ues[num_ues];
    int broker_rcv_accept_gnbs[num_gnbs];
    int tx_data_count = 0;
    int tx_count_lim = 0;
    float pl_ue_2 = 10.0f;

    int nbytes = buffer_vec.size() * sizeof(cf_t);
    int received_from_matlab = 0;
    int summ = 0;
    uint32_t dummy_gnb = 0;
    uint32_t dummy_ue = 0;
    bool gnb_rx_redy = false;
    bool ue_rx_redy = false;
    bool gnb_tx_samples_redy = false;
    bool ue_tx_samples_redy = false;
    int size_rep_gnb_rx = 0;
    int size_rep_ue_rx = 0;
    int counter = 0;
    double ue_ms_elapsed = 0;
    double gnb_ms_elapsed = 0;
    while (1)
    {        
        printf("-->> Получаем запрос от RX gNb\n");
        if(!gnb_rx_redy){
            size_rep_gnb_rx = zmq_recv(rep_socket_for_gnb_rx, &dummy_gnb, sizeof(dummy_gnb), ZMQ_DONTWAIT);
            if(size_rep_gnb_rx == -1){
                printf("!!!!!!!!!!!   -----1 rep_socket_for_gnb_rx\n");
                broker_rcv_accept_gnbs[0] = 0;
            } else{
                gnb_rx_redy = true;
                printf("broker received [buffer_acc] form GNB RX = %d dummy = %d\n", size_rep_gnb_rx, dummy_gnb);
            }
        }

        if(!ue_rx_redy){
            printf("-->> Получаем запрос от RX UE\n");
            size_rep_ue_rx = zmq_recv(rep_socket_for_ue_1_rx, &dummy_ue, sizeof(dummy_ue), ZMQ_DONTWAIT);
            if(size_rep_ue_rx == -1){
                printf("!!!!!!!!!!!   -----1 rep_socket_for_ue_1_rx\n");
                broker_rcv_accept_ues[0] = 0;
            } else{
                ue_rx_redy = true;
                printf("broker received [buffer_acc] from UE[1] RX = %d dummy_ue = %d\n", size_rep_ue_rx, dummy_ue);
            }
        }
        printf("--------------------------------------\n");

            
        printf("\n-->> Отправляем запрос и получаем СЭМПЛЫ от gNB\n");
        int send = zmq_send(req_socket_from_gnb_tx, &dummy_ue, size_rep_ue_rx, 0);
        printf("req_socket_from_gnb_tx [send] = %d\n", send);

        std::fill(buffer_vec.begin(), buffer_vec.end(), 0);
        int size_recv_req_tx_gnb = zmq_recv(req_socket_from_gnb_tx,  (void*)buffer_vec.data(), nbytes, ZMQ_DONTWAIT);
        if (size_recv_req_tx_gnb != -1)
        {
            gnb_tx_samples_redy = true;
            gnb_ms_elapsed +=  1000 * NBYTES2NSAMPLES(size_recv_req_tx_gnb) / 11520000;
            printf("[TIME - %d] samples elapsed in gNb = %d elapsed %lf [ms]", counter, NBYTES2NSAMPLES(size_recv_req_tx_gnb), gnb_ms_elapsed);
            //printf("broker received from gNb  bytes =  %d\n", size_recv_req_tx_gnb);
        } else {
            gnb_tx_samples_redy = false;
            printf("gnb_tx_samples_redy = false;\n");
        }

        printf("\n-->> Отправляем запрос и получаем СЭМПЛЫ от UE\n");
        send = zmq_send(req_socket_from_ue_1_tx, &dummy_gnb, size_rep_gnb_rx, 0);
        
        printf("req_socket_from_ue_1_tx [send] = %d\n", send);

        std::fill(buffer_recv_from_ue.begin(), buffer_recv_from_ue.end(), 0);
        int size_recv_req_tx_ue = zmq_recv(req_socket_from_ue_1_tx, (void*)buffer_recv_from_ue.data(), nbytes, ZMQ_DONTWAIT);
        if (size_recv_req_tx_ue  != -1)
        {
            ue_tx_samples_redy = true;
            ue_ms_elapsed +=  1000 * NBYTES2NSAMPLES(size_recv_req_tx_ue) / 11520000;
            printf("[TIME - %d] samples elapsed in UE = %d elapsed %lf [ms]", counter, NBYTES2NSAMPLES(size_recv_req_tx_ue),gnb_ms_elapsed);
            //printf("broker [received data] from UE[1] %d size packet\n", size_recv_req_tx_ue);
        } else {
            ue_tx_samples_redy = false;
            printf("ue_tx_samples_redy = false\n");
        }

        
        if(ue_rx_redy && gnb_tx_samples_redy){
            printf("\n-->> Отправляем СЭМПЛЫ от gNB до UE\n");
            send = zmq_send(rep_socket_for_ue_1_rx, (void*)buffer_vec.data(), size_recv_req_tx_gnb, 0);
            printf("rep_socket_for_ue_1_rx [send data] = %d\n", send);
            ue_rx_redy = false;
        }
        
        if(gnb_rx_redy && ue_tx_samples_redy){
            printf("\n-->> Отправляем СЭМПЛЫ от UE до gNB\n");
            send = zmq_send(rep_socket_for_gnb_rx, (void*)buffer_recv_from_ue.data(), size_recv_req_tx_ue, 0);
            printf("rep_socket_for_gnb_rx [send data] = %d\n", send);
            gnb_rx_redy = false;
        }
        counter++;
        printf("gNB [TIME - %lf] elapsed [ms]\n", gnb_ms_elapsed);
        printf("UE [TIME - %lf] elapsed [ms]\n", ue_ms_elapsed);
        // summ = 0;
        printf("-------ЗАКОНЧИЛИ ИТЕРАЦИЮ -----------\n");
    }

    zmq_close(rep_socket_for_gnb_rx);
    zmq_close (req_socket_from_ue_1_tx);

    return 0;
}
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


typedef _Complex float cf_t;
#define NSAMPLES2NBYTES(X) (((uint32_t)(X)) * sizeof(cf_t))
#define NBYTES2NSAMPLES(X) ((X) / sizeof(cf_t))
#define ZMQ_MAX_BUFFER_SIZE (NSAMPLES2NBYTES(3072000)) // 10 subframes at 20 MHz
#define NBYTES_PER_ONE_SAMPLE (NSAMPLES2NBYTES(1)) // 1 sample

int main(int argc, char *argv[]){
    std::cout << "ZMQ_MAX_BUFFER_SIZE = " << ZMQ_MAX_BUFFER_SIZE << std::endl;
    std::cout << "NBYTES_PER_ONE_SAMPLE = " << NBYTES_PER_ONE_SAMPLE << std::endl;
    std::cout << "sizeof(cf_t) = " << sizeof(cf_t) << std::endl;

    int ret = 0;

    int port_gnb_tx = 2000;
    int port_gnb_rx = 2001;

    int port_ue_1_tx = 2111;
    int port_ue_1_rx = 2110;
    int port_ue_2_tx = 2121;
    int port_ue_2_rx = 2120;

    std::string addr_recv_port_gnb_tx = "tcp://localhost:" + std::to_string(port_gnb_tx);
    std::string addr_recv_port_gnb_rx = "tcp://*:" + std::to_string(port_gnb_rx);

    std::string addr_recv_port_ue_1_tx = "tcp://localhost:" + std::to_string(port_ue_1_tx);
    std::string addr_send_port_ue_1_rx = "tcp://*:" + std::to_string(port_ue_1_rx);
    std::string addr_recv_port_ue_2_tx = "tcp://localhost:" + std::to_string(port_ue_2_tx);
    std::string addr_send_port_ue_2_rx = "tcp://*:" + std::to_string(port_ue_2_rx);

    // Initialize ZMQ
    void *context = zmq_ctx_new ();
    void *req_socket_from_gnb_tx = zmq_socket (context, ZMQ_REQ);

    void *req_socket_from_ue_1_tx = zmq_socket (context, ZMQ_REQ);
    void *req_socket_from_ue_2_tx = zmq_socket (context, ZMQ_REQ);

    ret = zmq_connect(req_socket_from_gnb_tx, addr_recv_port_gnb_tx.c_str());
    printf("ret = %d\n",ret);

    ret = zmq_connect(req_socket_from_ue_1_tx, addr_recv_port_ue_1_tx.c_str());
    printf("ret = %d\n",ret);
    ret = zmq_connect(req_socket_from_ue_2_tx, addr_recv_port_ue_2_tx.c_str());
    printf("ret = %d\n",ret);

    void *send_socket_for_gnb_rx = zmq_socket(context, ZMQ_REP);
    void *send_socket_for_ue_1_rx = zmq_socket(context, ZMQ_REP);
    void *send_socket_for_ue_2_rx = zmq_socket(context, ZMQ_REP);

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

    if(zmq_bind(send_socket_for_ue_2_rx, addr_send_port_ue_2_rx.c_str())) {
        perror("zmq_bind");
        return 1;
    } else {
        printf("zmq_bind [send_socket_for_ue_2_rx] - success\n");
    }

    using cf_t = std::complex<float>;
    std::vector<cf_t> buffer_vec(BUFFER_MAX * sizeof(cf_t));
    // buffer_vec(BUFFER_MAX * sizeof(cf_t));

    char buffer_acc[BUFFER_MAX];
    char buffer[BUFFER_MAX];
    char buffer_ul[BUFFER_MAX];
    char buffer_to_gnb[BUFFER_MAX];
    char buffer_to_ue_2[BUFFER_MAX];

    char buffer_complex[BUFFER_MAX];

    
    // buffer = (char*)malloc(sizeof(char) * ZMQ_MAX_BUFFER_SIZE);
    // buffer_ul = (char*)malloc(sizeof(char) * ZMQ_MAX_BUFFER_SIZE);
    // buffer_to_gnb = (char*)malloc(sizeof(char) * ZMQ_MAX_BUFFER_SIZE);
    // buffer_to_ue_2 = (char*)malloc(sizeof(char) * ZMQ_MAX_BUFFER_SIZE);
    // buffer_complex = (char*)malloc(sizeof(char) * ZMQ_MAX_BUFFER_SIZE);
    int size;
    int size_ue_2;
    int size_recv;
    int size_from_ue_tx;

    int num_ues = 2;
    int num_gnbs = 1;
    int broker_rcv_accept_ues[num_ues];
    int broker_rcv_accept_gnbs[num_gnbs];
    int tx_data_count = 0;
    int tx_count_lim = 0;

    int nbytes      = buffer_vec.size() * sizeof(cf_t);

    while(1){
        // receive conn accept from RX's
        memset(buffer_acc, 0, sizeof(buffer_acc));
        size = zmq_recv(send_socket_for_ue_1_rx, buffer_acc, sizeof(buffer_acc), 0);
        if(size == -1){
            printf("broker received %d size packet id [%d]\n", size);
            continue;
        } else{
            broker_rcv_accept_ues[0] = 1;
        }
        // memset(buffer, 0, sizeof(buffer));
        // if (tx_data_count > tx_count_lim){
        //     size = zmq_recv(send_socket_for_ue_2_rx, buffer, sizeof(buffer), 0);
        //     if(size == -1){
        //         printf("broker received %d size packet id [%d]\n", size);
        //         continue;
        //     } else{
        //         broker_rcv_accept_ues[1] = 1;
        //     }
        // }

        memset(buffer_acc, 0, sizeof(buffer_acc));
        size = zmq_recv(send_socket_for_gnb_rx, buffer_acc, sizeof(buffer_acc), 0);
        if(size == -1){
            printf("broker received %d size packet id [%d]\n", size);
            continue;
        } else{
            broker_rcv_accept_gnbs[0] = 1;
        }

        int summ = 0;
        for (int i = 0; i < num_ues; i++){
            summ += broker_rcv_accept_ues[i];
        }
        if (summ >= 1  && broker_rcv_accept_gnbs[0] == num_gnbs)
        {
            // send accepts to TX's
            zmq_send(req_socket_from_gnb_tx, buffer_acc, size, 0);
            zmq_send(req_socket_from_ue_1_tx, buffer_acc, size, 0);

            // if (tx_data_count >= tx_count_lim){
            //     zmq_send(req_socket_from_ue_2_tx, buffer, size, 0);
            // }

            // start data transmissiona
            memset(buffer_complex, 0, sizeof(buffer_complex));
            size = zmq_recv(req_socket_from_gnb_tx,  (void*)buffer_vec.data(), nbytes, 0);
            
            if (size != -1)
            {
                printf("broker received from gNb =  %d size packet\n", size);
                // for (int i = 0; i < buffer_vec.size(); i++){
                //     if(buffer_vec[i].real() > 1){
                //         std::cout << "real = " << buffer_vec[i].real()  << "imag = " << buffer_vec[i].imag() << std::endl;
                //     }
                //     // sleep(10);
                // }
                
            }
            zmq_send(send_socket_for_ue_1_rx, (void*)buffer_vec.data(), size, 0);



            // memset(buffer_to_ue_2, 0, sizeof(buffer_to_ue_2));
            // if (tx_data_count >=tx_count_lim){
            //     for (int i = 0; i < size; i++){
            //         buffer_to_ue_2[i] = (int8_t)(buffer[i] / 10.0f);
            //     }
            //     zmq_send(send_socket_for_ue_2_rx, buffer_to_ue_2, size, 0);
            // }
            
            // TODO: Concatenate samples
            memset(buffer, 0, sizeof(buffer));
            std::fill(buffer_vec.begin(), buffer_vec.end(), 0);
            size = zmq_recv(req_socket_from_ue_1_tx, (void*)buffer_vec.data(), nbytes, 0);
            if (size != -1)
            {
                printf("broker received from UE[1] %d size packet\n", size);
            }
            
            // memset(buffer_ul, 0, sizeof(buffer_ul));
            // if (tx_data_count >= tx_count_lim){
            //     size_ue_2 = zmq_recv(req_socket_from_ue_2_tx, buffer_ul, sizeof(buffer_ul), 0);
            //     if (size != -1)
            //     {
            //         printf("broker received from UE[2] %d size packet\n", size_ue_2);
            //     }
            //     for (int i = 0; i < size_ue_2; i++)
            //     {
            //         buffer_ul[i] = (int8_t)(buffer_ul[i]  / 10.0f);
            //     }

            // }

            // memset(buffer_to_gnb, 0, sizeof(buffer_to_gnb));

            // int max_size = size;
            // if (size_ue_2 >= size)
            // {
            //     max_size = size_ue_2;
            // }

            // printf("size = %d, size_ue_2 = %d\n", size, size_ue_2);
            
            // // concatenate two buffers and send it to gNb
            // for (int i = 0; i < max_size; i++)
            // {
            //     buffer_to_gnb[i] = (buffer[i]) + (buffer_ul[i]);
            // }

            zmq_send(send_socket_for_gnb_rx, (void*)buffer_vec.data(), size, 0);

            tx_data_count++;
        }
        else
        {
            continue;
        }
    }

    free(buffer_complex);
    free(buffer);
    free(buffer_ul);
    free(buffer_to_gnb);
    free(buffer_to_ue_2);
    zmq_close(send_socket_for_gnb_rx);
    zmq_close (req_socket_from_ue_1_tx);

    return 0;
}
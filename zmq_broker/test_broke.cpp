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

void my_handler(int s){
    printf("Caught signal %d\n",s);
    
    exit(1); 

}

#define BUFFER_MAX 1024 * 1024


typedef _Complex float cf_t;
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

    std::cout << "ZMQ_MAX_BUFFER_SIZE = " << ZMQ_MAX_BUFFER_SIZE << std::endl;
    std::cout << "NBYTES_PER_ONE_SAMPLE = " << NBYTES_PER_ONE_SAMPLE << std::endl;
    std::cout << "sizeof(cf_t) = " << sizeof(cf_t) << std::endl;

    int ret = 0;
    int num_ues = 2;

    std::vector<int> ports_ue_tx = {2101, 2201}; //В теории парсим
    std::vector<int> ports_ue_rx = {2100, 2200}; //В теории парсим
    int port_gnb_tx = 2000;
    int port_gnb_rx = 2001;

    std::string addr_send_port_gnb_tx = "tcp://localhost:" + std::to_string(port_gnb_tx);
    std::string addr_recv_port_gnb_rx = "tcp://*:" + std::to_string(port_gnb_rx);

    //Address
    std::vector<std::string> addr_recv_ports_ue_rx(num_ues);
    std::vector<std::string> addr_send_ports_ue_tx(num_ues);


    for(int i = 0; i < num_ues; i++){
        addr_recv_ports_ue_rx[i] = "tcp://localhost:" + std::to_string(ports_ue_tx[i]);
        addr_send_ports_ue_tx[i] = "tcp://*:" + std::to_string(ports_ue_rx[i]);
    }

    // Initialize ZMQ
    void *context = zmq_ctx_new ();

    //gnb sockets
    void *req_socket_from_gnb_tx = zmq_socket (context, ZMQ_REQ);
    zmq_connect(req_socket_from_gnb_tx, addr_send_port_gnb_tx.c_str()); //gnb tx

    void *send_socket_for_gnb_rx = zmq_socket(context, ZMQ_REP); //gnb rx
    zmq_bind(send_socket_for_gnb_rx, addr_recv_port_gnb_rx.c_str());

    //ue sockets
    std::vector<void*>req_sockets_from_ue_tx(num_ues);
    std::vector<void*>send_sockets_for_ue_rx(num_ues);   

    for(int i = 0; i < num_ues; i++){
        //recv sockets
        req_sockets_from_ue_tx[i] = zmq_socket (context, ZMQ_REQ);
        zmq_connect(req_sockets_from_ue_tx[i], addr_recv_ports_ue_rx[i].c_str());

        //send socket
        send_sockets_for_ue_rx[i] = zmq_socket(context, ZMQ_REP);
        zmq_bind(send_sockets_for_ue_rx[i], addr_send_ports_ue_tx[i].c_str());
    }


    using cf_t = std::complex<float>;
    int N = 100000;
    std::vector<cf_t> buffer_vec(N);
    std::vector<cf_t> buffer_vec_ue_2(N);
    std::vector<cf_t> buffer_vec_to_gnb(N);

    char buffer_acc[10];
    char buffer[BUFFER_MAX];
    int size = 0;
    int size_ue_2 = 0;
    int size_recv = 0;
    int size_from_ue_tx = 0;

    int num_gnbs = 1;
    int broker_rcv_accept_ues[num_ues];
    int broker_rcv_accept_gnbs[num_gnbs];
    int tx_data_count = 0;
    int tx_count_lim = 0;
    float pl_ue_2 = 10.0f;

    int nbytes = buffer_vec.size() * sizeof(cf_t);

    while(1){
        // receive conn accept from RX's

        memset(buffer_acc, 0, sizeof(buffer_acc));
        size = zmq_recv(send_socket_for_gnb_rx, buffer_acc, sizeof(buffer_acc), 0);
        if(size == -1){
            printf("!!!!!!!!!!!   -----1 send_socket_for_gnb_rx\n");
            continue;
        } else{
            broker_rcv_accept_gnbs[0] = 1;
            printf("broker received [buffer_acc] form GNB RX = %d\n", size);
        }


    //     memset(buffer_acc, 0, sizeof(buffer_acc));
    //     size = zmq_recv(send_socket_for_ue_1_rx, buffer_acc, sizeof(buffer_acc), 0);
    //     if(size == -1){
    //         printf("!!!!!!!!!!!   -----1 send_socket_for_ue_2_rx\n");
    //         continue;
    //     } else{
    //         broker_rcv_accept_ues[0] = 1;
    //         printf("broker received [buffer_acc] from UE[1] RX = %d\n", size);
    //     }
    //     memset(buffer_acc, 0, sizeof(buffer_acc));
    //     if (tx_data_count >= tx_count_lim){
    //         size = zmq_recv(send_socket_for_ue_2_rx, buffer_acc, sizeof(buffer_acc), 0);
    //         if(size == -1){
    //             printf("!!!!!!!!!!!   -----1 send_socket_for_ue_2_rx\n");
    //             continue;
    //         } else{
    //             broker_rcv_accept_ues[1] = 1;
    //             printf("broker received [buffer_acc] from UE[2] RX = %d\n", size);
    //         }
    //     }

    //     int summ = 0;
    //     for (int i = 0; i < num_ues; i++){
    //         summ += broker_rcv_accept_ues[i];
    //     }
    //     if (summ >= 1  && broker_rcv_accept_gnbs[0] == num_gnbs)
    //     {
    //         int send = 0;
    //         // send accepts to TX's
    //         send = zmq_send(req_socket_from_gnb_tx, buffer_acc, size, 0);
    //         printf("req_socket_from_gnb_tx [send] = %d\n", send);
    //         send = zmq_send(req_socket_from_ue_1_tx, buffer_acc, size, 0);
    //         printf("req_socket_from_ue_1_tx [send] = %d\n", send);

    //         if (tx_data_count >= tx_count_lim){
    //             send = zmq_send(req_socket_from_ue_2_tx, buffer_acc, size, 0);
    //             printf("req_socket_from_ue_2_tx [send] = %d\n", send);
    //         }


    //         // start data transmissiona
    //         std::fill(buffer_vec.begin(), buffer_vec.end(), 0);
    //         size = zmq_recv(req_socket_from_gnb_tx,  (void*)buffer_vec.data(), nbytes, 0);
    //         if (size != -1)
    //         {
    //             printf("broker received from gNb =  %d size packet buffer size = %d\n", size,buffer_vec.size());
    //         }
    //         send = zmq_send(send_socket_for_ue_1_rx, (void*)buffer_vec.data(), size, 0);
    //         printf("send_socket_for_ue_1_rx [send data] = %d\n", send);


    //         std::fill(buffer_vec_ue_2.begin(), buffer_vec_ue_2.end(), 0);
    //         if (tx_data_count >=tx_count_lim){
    //             for (int i = 0; i < buffer_vec.size(); i++){
    //                 // std::complex<float> val;
    //                 // val = std::complex<float>(buffer_vec[i].real()/pl_ue_2, buffer_vec[i].imag()/pl_ue_2);
    //                 buffer_vec_ue_2[i] = buffer_vec[i] / pl_ue_2;
    //             }
    //             send = zmq_send(send_socket_for_ue_2_rx, (void*)buffer_vec_ue_2.data(), size, 0);
    //             printf("send_socket_for_ue_2_rx [send data] = %d\n", send);
    //         }

    //         usleep(50000);
    //         // TODO: Concatenate samples
    //         std::fill(buffer_vec.begin(), buffer_vec.end(), 0);
    //         size = zmq_recv(req_socket_from_ue_1_tx, (void*)buffer_vec.data(), nbytes, 0);
    //         if (size != -1)
    //         {
    //             printf("broker [received data] from UE[1] %d size packet\n", size);
    //         }

    //         std::fill(buffer_vec_to_gnb.begin(), buffer_vec_to_gnb.end(), 0);
    //         if (tx_data_count >= tx_count_lim){
    //             std::fill(buffer_vec_ue_2.begin(), buffer_vec_ue_2.end(), 0);
    //             size_ue_2 = zmq_recv(req_socket_from_ue_2_tx, (void*)buffer_vec_ue_2.data(), nbytes, 0);
    //             if (size_ue_2 != -1)
    //             {
    //                 printf("broker [received data] from UE[2] %d size packet\n", size_ue_2);
    //                 for (int i = 0; i < buffer_vec_ue_2.size(); i++){
    //                     std::complex<float> val_1;
    //                     val_1 = std::complex<float>(buffer_vec_ue_2[i].real()/pl_ue_2, buffer_vec_ue_2[i].imag()/pl_ue_2);
    //                     buffer_vec_ue_2[i] = val_1;
    //                     //buffer_vec_to_gnb[i] = buffer_vec_ue_2[i] * pl_ue_2;
    //                 }
    //             } else {
    //                 printf("!!!!!!!!!!!   -----1 req_socket_from_ue_2_tx\n");
    //             }
    //         }

    //         int max_size = size;
    //         std::transform(buffer_vec_ue_2.begin(), buffer_vec_ue_2.end(), buffer_vec.begin(), buffer_vec_ue_2.begin(), std::plus<std::complex<float>>());

    //         printf("size = %d, size_ue_2 = %d\n", size, size_ue_2);

    //         send = zmq_send(send_socket_for_gnb_rx, (void*)buffer_vec_ue_2.data(), max_size, 0);
    //         printf("send_socket_for_gnb_rx [send data] = %d\n", send);

    //         tx_data_count++;
    //     }
    //     else
    //     {
    //         continue;
    //     }
    // }

    // zmq_close(send_socket_for_gnb_rx);
    // zmq_close (req_socket_from_ue_1_tx);

    // return 0;
}
}
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
    int N = BUFFER_MAX;
    std::vector<cf_t> buffer_vec(N);
    std::vector<cf_t> buffer_vec_ue_2(N);
    std::vector<cf_t> buffer_vec_to_gnb(N);

    char buffer_acc[10];

    int size = 0;
    int size_ue_2 = 0;
    int size_recv = 0;
    int size_from_ue_tx = 0;

    int num_ues = 2;
    int num_gnbs = 1;
    int broker_rcv_accept_ues[num_ues];
    int broker_rcv_accept_gnbs[num_gnbs];
    int tx_data_count = 0;
    int tx_count_lim = 100;
    float pl_ue_2 = 30.0f;

    int nbytes = buffer_vec.size() * sizeof(cf_t);

    while(1){
        // receive conn accept from RX's
        memset(buffer_acc, 0, sizeof(buffer_acc));
        size = zmq_recv(send_socket_for_ue_1_rx, buffer_acc, sizeof(1), 0);
        if(size == -1){
            printf("!!!!!!!!!!!   -----1\n");
            continue;
        } else{
            broker_rcv_accept_ues[0] = 1;
            printf("broker received from UE[1] RX = %d\n", size);
        }
        memset(buffer_acc, 0, sizeof(buffer_acc));
        if (tx_data_count > tx_count_lim){
            size = zmq_recv(send_socket_for_ue_2_rx, buffer_acc, sizeof(1), 0);
            if(size == -1){
                printf("!!!!!!!!!!!   -----1 send_socket_for_ue_2_rx\n");
                continue;
            } else{
                broker_rcv_accept_ues[1] = 1;
                printf("broker received ACCEPT UE[2] RX = %d\n", size);
            }
        }

        memset(buffer_acc, 0, sizeof(buffer_acc));
        size = zmq_recv(send_socket_for_gnb_rx, buffer_acc, sizeof(1), 0);
        if(size == -1){
            printf("!!!!!!!!!!!   -----1 send_socket_for_gnb_rx\n");
            continue;
        } else{
            broker_rcv_accept_gnbs[0] = 1;
            printf("broker received ACCEPT form GNB RX = %d\n", size);
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

            if (tx_data_count >= tx_count_lim){
                zmq_send(req_socket_from_ue_2_tx, buffer_acc, size, 0);
            }

            // start data transmissiona
            //std::fill(buffer_vec.begin(), buffer_vec.end(), 0);
            size = zmq_recv(req_socket_from_gnb_tx,  (void*)buffer_vec.data(), nbytes, 0);
            if (size != -1)
            {
                printf("broker received from gNb =  %d size packet buffer size = %d\n", size,buffer_vec.size());
                // for (int i = 0; i < buffer_vec.size(); i++){
                //     if(buffer_vec[i].real() > 1 || buffer_vec[i].real() < -1){
                //         std::cout << "real = " << buffer_vec[i].real()  << "imag = " << buffer_vec[i].imag() << std::endl;
                //     }
                //     // sleep(10);
                // }
                // if(size == 46080){
                //     for (int i = 0; i < size / 8; i++){
                //         std::cout << "real = " << buffer_vec[i].real() << "imag = " << buffer_vec[i].imag() << std::endl;
                //     }   
                // }
                
            }
            zmq_send(send_socket_for_ue_1_rx, (void*)buffer_vec.data(), size, 0);


            std::fill(buffer_vec_ue_2.begin(), buffer_vec_ue_2.end(), 0);
            if (tx_data_count >=tx_count_lim){
                for (int i = 0; i < buffer_vec.size(); i++){
                    // std::complex<float> val;
                    // val = std::complex<float>(buffer_vec[i].real()/pl_ue_2, buffer_vec[i].imag()/pl_ue_2);
                    buffer_vec_ue_2[i] = buffer_vec[i] / pl_ue_2;
                }
                zmq_send(send_socket_for_ue_2_rx, (void*)buffer_vec_ue_2.data(), size, 0);
            }

            // TODO: Concatenate samples
            std::fill(buffer_vec.begin(), buffer_vec.end(), 0);
            size = zmq_recv(req_socket_from_ue_1_tx, (void*)buffer_vec.data(), nbytes, 0);
            if (size != -1)
            {
                printf("broker received from UE[1] %d size packet\n", size);
            }

            std::fill(buffer_vec_to_gnb.begin(), buffer_vec_to_gnb.end(), 0);
            if (tx_data_count >= tx_count_lim){
                std::fill(buffer_vec_ue_2.begin(), buffer_vec_ue_2.end(), 0);
                size_ue_2 = zmq_recv(req_socket_from_ue_2_tx, (void*)buffer_vec_ue_2.data(), nbytes, 0);
                if (size_ue_2 != -1)
                {
                    printf("broker received from UE[2] %d size packet\n", size_ue_2);
                    for (int i = 0; i < size_ue_2/8; i++){
                        std::complex<float> val_1;
                        val_1 = std::complex<float>(buffer_vec_ue_2[i].real()/pl_ue_2, buffer_vec_ue_2[i].imag()/pl_ue_2);
                        buffer_vec_ue_2[i] = val_1;
                        //buffer_vec_to_gnb[i] = buffer_vec_ue_2[i] * pl_ue_2;
                    }
                } else {
                    printf("!!!!!!!!!!!   -----1 req_socket_from_ue_2_tx\n");
                }
            }

            int max_size = size;
            std::fill(buffer_vec_to_gnb.begin(), buffer_vec_to_gnb.end(), 0);
            if (size_ue_2 >= size)
            {
                max_size = size_ue_2;
                printf("!!! SIZES ARE NOT EQUAL ...size = %d, size_ue_2 = %d\n", size, size_ue_2);
                for (int i = 0; i < max_size/8; i++){
                    std::complex<float> val_2;
                    val_2 = std::complex<float>(buffer_vec_ue_2[i].real() + buffer_vec[i].real(),  buffer_vec_ue_2[i].imag() + buffer_vec[i].imag());
                    buffer_vec_to_gnb[i] = val_2;
                    //buffer_vec_to_gnb[i] += buffer_vec[i];
                }
                
            } else {
                for (int i = 0; i < max_size/8; i++){
                    std::complex<float> val_2;
                    val_2 = std::complex<float>(buffer_vec_ue_2[i].real() + buffer_vec[i].real(),  buffer_vec_ue_2[i].imag() + buffer_vec[i].imag());
                    buffer_vec_to_gnb[i] = val_2;
                    //buffer_vec_to_gnb[i] += buffer_vec[i];
                }
            }

            printf("size = %d, size_ue_2 = %d\n", size, size_ue_2);
            
            // concatenate two buffers and send it to gNb
            

            zmq_send(send_socket_for_gnb_rx, (void*)buffer_vec_to_gnb.data(), max_size, 0);

            tx_data_count++;
        }
        else
        {
            continue;
        }
    }

    zmq_close(send_socket_for_gnb_rx);
    zmq_close (req_socket_from_ue_1_tx);

    return 0;
}
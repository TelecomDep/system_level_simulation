#include <iostream>
#include <mutex>
#include <zmq.h>
#include <thread>
#include <cstring>
#include <cmath>
#include <unistd.h>
#include <cstdio>

#include <vector>
#include <random>
#include <complex>
#include <algorithm>

#include <gnuradio/top_block.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/types.h>

#include <gnuradio/blocks/add_blk.h> 

void my_handler(int s){
    printf("Caught signal %d\n",s);
    
    exit(1); 

}

#define BUFFER_MAX 1024 * 1024

using cf_t = gr_complex;
#define NSAMPLES2NBYTES(X) (((uint32_t)(X)) * sizeof(cf_t))
#define NBYTES2NSAMPLES(X) ((X) / sizeof(cf_t))
#define ZMQ_MAX_BUFFER_SIZE (NSAMPLES2NBYTES(3072000)) // 10 subframes at 20 MHz
#define NBYTES_PER_ONE_SAMPLE (NSAMPLES2NBYTES(1)) // 1 sample

std::vector<cf_t> multiply_by_const(const std::vector<cf_t>& input, float coeff) {
    auto tb = gr::make_top_block("multiply_tmp");
    auto src = gr::blocks::vector_source<gr_complex>::make(input, false);
    auto mult = gr::blocks::multiply_const_cc::make(coeff);
    auto sink = gr::blocks::vector_sink<gr_complex>::make();
    
    tb->connect(src, 0, mult, 0);
    tb->connect(mult, 0, sink, 0);
    tb->start();
    tb->wait();
    
    return sink->data();
}

std::vector<cf_t> add_vectors_gnuradio(const std::vector<cf_t>& vec1, const std::vector<cf_t>& vec2) {
    auto tb = gr::make_top_block("add_vectors");
    auto src1 = gr::blocks::vector_source_c::make(vec1, false);
    auto src2 = gr::blocks::vector_source_c::make(vec2, false);
    
    auto add = gr::blocks::add_cc::make();
    
    auto sink = gr::blocks::vector_sink_c::make();
    
    tb->connect(src1, 0, add, 0);
    tb->connect(src2, 0, add, 1);
    tb->connect(add, 0, sink, 0);
    
    tb->start();
    tb->wait();
    
    return sink->data();
}

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
    int sr = 11520000;

    int port_gnb_tx = 2000;
    int port_gnb_rx = 2001;

    int ue_num = 2;
    // std::vector<int> port_ue_tx{2111, 2121, 2131, 2141, 2151, 2161, 2171, 2101, 2201, 2301};
    // std::vector<int> port_ue_rx{2110, 2120, 2130, 2140, 2150, 2160, 2170, 2100, 2200, 2300};
    std::vector<int> port_ue_tx{2101, 2201, 2301, 2111, 2131, 2151, 2161, 2171, 2101, 2201, 2301};
    std::vector<int> port_ue_rx{2100, 2200, 2300, 2110, 2130, 2150, 2160, 2170, 2100, 2200, 2300};


    std::string addr_recv_port_gnb_tx = "tcp://localhost:" + std::to_string(port_gnb_tx);
    std::string addr_recv_port_gnb_rx = "tcp://*:" + std::to_string(port_gnb_rx);

    std::vector<std::string> addr_recv_port_ue_tx(ue_num);
    std::vector<std::string> addr_send_port_ue_rx(ue_num);

    for(int i = 0; i < ue_num; i++){
        addr_recv_port_ue_tx[i] = "tcp://localhost:" + std::to_string(port_ue_tx[i]);
        addr_send_port_ue_rx[i] = "tcp://*:" + std::to_string(port_ue_rx[i]);
    }

    // Initialize ZMQ
    void *context = zmq_ctx_new ();

    //socket work UE
    std::vector<void*> req_sockets_from_ue_tx(ue_num);
    std::vector<void*> send_sockets_for_ue_rx(ue_num);
    for(int i = 0; i < ue_num; i++){
        req_sockets_from_ue_tx[i] = zmq_socket(context, ZMQ_REQ);

        ret = zmq_connect(req_sockets_from_ue_tx[i], addr_recv_port_ue_tx[i].c_str());
        printf("ret = %d\n",ret);

        send_sockets_for_ue_rx[i] = zmq_socket(context, ZMQ_REP);

        if(zmq_bind(send_sockets_for_ue_rx[i], addr_send_port_ue_rx[i].c_str())) {
        perror("zmq_bind");
        return 1;
        } else {
            printf("zmq_bind [send_socket_for_ue_2_rx] - success\n");
        }

    }

    //socket work gNB
    void *req_socket_from_gnb_tx = zmq_socket (context, ZMQ_REQ);
    ret = zmq_connect(req_socket_from_gnb_tx, addr_recv_port_gnb_tx.c_str());
    printf("ret = %d\n",ret);

    void *send_socket_for_gnb_rx = zmq_socket(context, ZMQ_REP);
    if(zmq_bind(send_socket_for_gnb_rx, addr_recv_port_gnb_rx.c_str())) {
        perror("zmq_bind");
        return 1;
    } else {
        printf("zmq_bind [send_socket_for_gnb_rx]- success\n");
    }

    using cf_t = std::complex<float>;
    int N = 100000;
    std::vector<cf_t> buffer_vec(N);
    std::vector<cf_t> concatenated(N, 0);
    std::vector<cf_t> buffer_vec_to_gnb(N);
    std::vector<std::vector<cf_t>> all_buffer_vec(ue_num, std::vector<cf_t>(N));
    std::vector<int> sizes(ue_num, 0);

    //std::vector<cf_t> concatenated(N);

    char buffer_acc[10];
    char buffer[BUFFER_MAX];
    int size = 0;
    int size_ue_2 = 0;
    int size_recv = 0;
    int size_from_ue_tx = 0;

    int num_gnbs = 1;
    std::vector<int> broker_rcv_accept_ues(ue_num, 0);
    std::vector<int> broker_rcv_accept_gnbs(num_gnbs, 0);
    int tx_data_count = 0;
    int tx_count_lim = 0;
    float pl_ue_2 = 10.0f;

    int nbytes = buffer_vec.size() * sizeof(cf_t);

    FILE* ue1_log = fopen("/home/iluha/srsRAN/srsRAN_Project/logs/ue1_broker_sizes.log", "w");
    FILE* gnb_log = fopen("/home/iluha/srsRAN/srsRAN_Project/logs/gnb_broker_sizes.log", "w");
    std::vector<float> pl_ue = {1.0f, 0.1f};

    while(1){
        memset(buffer_acc, 0, sizeof(buffer_acc));
        size = zmq_recv(send_socket_for_gnb_rx, buffer_acc, sizeof(buffer_acc), 0);
        if(size == -1){
            printf("!!!!!!!!!!!   -----1 send_socket_for_gnb_rx\n");
            continue;
        } else{
            broker_rcv_accept_gnbs[0] = 1;
            printf("broker received [buffer_acc] form GNB RX = %d\n", size);
        }

        for(int i = 0; i < ue_num; i++){
            memset(buffer_acc, 0, sizeof(buffer_acc));
            size = zmq_recv(send_sockets_for_ue_rx[i], buffer_acc, sizeof(buffer_acc), 0);
            if(size == -1){
                printf("!!!!!!!!!!!   -----1 send_socket_for_ue_%d_rx\n", i);
                continue;
            } else{
                broker_rcv_accept_ues[i] = 1;
                printf("broker received [buffer_acc] from UE[1] RX = %d\n", size);
            }
            
        }

        int summ = 0;
        for (int i = 0; i < ue_num; i++){
            summ += broker_rcv_accept_ues[i];
        }
        if (summ == ue_num  && broker_rcv_accept_gnbs[0] == num_gnbs)
        {
            int send = 0;
            // send accepts to TX's
            send = zmq_send(req_socket_from_gnb_tx, buffer_acc, size, 0);
            printf("req_socket_from_gnb_tx [send] = %d\n", send);

            
            for(int i = 0; i < ue_num; i++){
                send = zmq_send(req_sockets_from_ue_tx[i], buffer_acc, size, 0);
                printf("req_socket_from_ue[%d]_tx [send] = %d\n", i, send);
            }

            // start data transmissiona
            std::fill(buffer_vec.begin(), buffer_vec.end(), 0);
            size = zmq_recv(req_socket_from_gnb_tx,  (void*)buffer_vec.data(), nbytes, 0);
            // fprintf(gnb_log, "size %d number: %d, sizeof: %d\n", size, size/sizeof(cf_t), sizeof(cf_t));
            if (size != -1)
            {
                printf("broker received from gNb =  %d size packet buffer size = %d\n", size,buffer_vec.size());
            }   
            int buff_size = size / sizeof(cf_t);
            for(int i = 0; i < ue_num; i++){
                // std::vector<cf_t> tmp_buff(size / sizeof(cf_t));
                // for(int j = 0; j < size / sizeof(cf_t); j++){
                //     all_buffer_vec[i][j] = buffer_vec[j] * pl_ue[i];
                // }

                std::vector<cf_t> tmp_buff = multiply_by_const(buffer_vec, pl_ue[i]);
                send = zmq_send(send_sockets_for_ue_rx[i], (void*)tmp_buff.data(), size, 0);
                // usleep(200);
                printf("send_socket_for_ue_%d_rx [send data] = %d\n", i, send);
            }

            usleep(15000);
            // TODO: Concatenate samples
            for(int i = 0; i < ue_num; i++){
                std::fill(all_buffer_vec[i].begin(), all_buffer_vec[i].end(), 0);
                size = zmq_recv(req_sockets_from_ue_tx[i], (void*)all_buffer_vec[i].data(), nbytes, 0);
                // fprintf(ue1_log, "index %d size %d\n", i, size);
                if (size != -1)
                {
                    // float coeff = pl_ue[i];
                    // for(int j = 0; j < size / sizeof(cf_t); j++){
                    //     std::complex<float> val;
                    //     val = std::complex<float>(all_buffer_vec[i][j].real()/ coeff, all_buffer_vec[i][j].imag()/coeff);
                    //     all_buffer_vec[i][j] = val;
                    // }
                    all_buffer_vec[i] = multiply_by_const(all_buffer_vec[i], pl_ue[i]);
                    sizes[i] = size;
                } 
            }
            // for(int i = 0; i < ue_num; ++i){
            //     for(int j = 0; j < sizes[i] / sizeof(cf_t); ++j){
            //         concatenated[j] += all_buffer_vec[i][j];
            //     }
            // }
            // std::transform(all_buffer_vec[1].begin(), all_buffer_vec[1].end(), all_buffer_vec[0].begin(), all_buffer_vec[1].begin(), std::plus<std::complex<float>>());
            // std::transform(all_buffer_vec[1].begin(), all_buffer_vec[1].end(), all_buffer_vec[0].begin(), all_buffer_vec[1].begin(), std::plus<std::complex<float>>());
            size_t max_samples = std::max(sizes[0], sizes[1]);

            auto result = add_vectors_gnuradio(all_buffer_vec[0], all_buffer_vec[1]);
            send = zmq_send(send_socket_for_gnb_rx, (void*)result.data(), sizes[0], 0);
            // printf("send_socket_for_gnb_rx [send data] = %d\n", send);
            tx_data_count++;
        }
        else
        {
            continue;
        }
    }
    zmq_close(send_socket_for_gnb_rx);
    zmq_close (req_sockets_from_ue_tx[0]);
    zmq_close (req_sockets_from_ue_tx[1]);

    return 0;
}
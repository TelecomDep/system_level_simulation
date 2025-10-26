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

#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

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

struct equipment {
    int id;
    int type;
    int rx_port;
    int tx_port;
};

// Define how to convert a Person object to JSON
void to_json(json& j, const equipment& p) {
    j = json{{"id", p.id}, {"type", p.type}, {"rx_port", p.rx_port}, {"tx_port", p.tx_port}};
}

// Define how to convert JSON to a Person object
void from_json(const json& j, equipment& p) {
    j.at("id").get_to(p.id);
    j.at("type").get_to(p.type);
    j.at("rx_port").get_to(p.rx_port);
    j.at("tx_port").get_to(p.tx_port);
}


int main(){

    std::ifstream f("../configs/broker.json");
    json data = json::parse(f);

    std::string s = data.dump();
    std::cout << "json size: " << data.size() << std::endl << "data.dump() = " << s << std::endl;

    return 0;
}
// int main(int argc, char *argv[]){

//     struct sigaction sigIntHandler;

//     sigIntHandler.sa_handler = my_handler;
//     sigemptyset(&sigIntHandler.sa_mask);
//     sigIntHandler.sa_flags = 0;
 
//     sigaction(SIGINT, &sigIntHandler, NULL);

//     std::cout << "ZMQ_MAX_BUFFER_SIZE = " << ZMQ_MAX_BUFFER_SIZE << std::endl;
//     std::cout << "NBYTES_PER_ONE_SAMPLE = " << NBYTES_PER_ONE_SAMPLE << std::endl;
//     std::cout << "sizeof(cf_t) = " << sizeof(cf_t) << std::endl;

//     int ret = 0;

//     int port_gnb_tx = 2000;
//     int port_gnb_rx = 2001;

//     int port_ue_1_tx = 2111;
//     int port_ue_1_rx = 2110;

//     int matlab_port = 4001;

//     std::string addr_recv_port_gnb_tx = "tcp://localhost:" + std::to_string(port_gnb_tx);
//     std::string addr_recv_port_gnb_rx = "tcp://*:" + std::to_string(port_gnb_rx);

//     std::string addr_recv_port_ue_1_tx = "tcp://localhost:" + std::to_string(port_ue_1_tx);
//     std::string addr_send_port_ue_1_rx = "tcp://*:" + std::to_string(port_ue_1_rx);

//     std::string matlab_server = "tcp://localhost:" + std::to_string(matlab_port);

//     // Initialize ZMQ
//     void *context = zmq_ctx_new ();
//     void *req_socket_from_gnb_tx = zmq_socket (context, ZMQ_REQ);
//     void *req_socket_from_ue_1_tx = zmq_socket (context, ZMQ_REQ);
//     void *matlab_server_req_socket = zmq_socket (context, ZMQ_REQ);

//     ret = zmq_connect(req_socket_from_gnb_tx, addr_recv_port_gnb_tx.c_str());
//     printf("ret = %d\n",ret);

//     ret = zmq_connect(req_socket_from_ue_1_tx, addr_recv_port_ue_1_tx.c_str());
//     printf("ret = %d\n",ret);

//     ret = zmq_connect(matlab_server_req_socket, matlab_server.c_str());
//     printf("Connection to matlab server ret = %d\n",ret);

//     void *send_socket_for_gnb_rx = zmq_socket(context, ZMQ_REP);
//     void *send_socket_for_ue_1_rx = zmq_socket(context, ZMQ_REP);

//     if(zmq_bind(send_socket_for_gnb_rx, addr_recv_port_gnb_rx.c_str())) {
//         perror("zmq_bind");
//         return 1;
//     } else {
//         printf("zmq_bind [send_socket_for_gnb_rx]- success\n");
//     }

//     if(zmq_bind(send_socket_for_ue_1_rx, addr_send_port_ue_1_rx.c_str())) {
//         perror("zmq_bind");
//         return 1;
//     } else {
//         printf("zmq_bind [send_socket_for_ue_1_rx] - success\n");
//     }

//     using cf_t = std::complex<float>;
//     int N = 100000;
//     std::vector<cf_t> buffer_vec(N);
//     std::vector<cf_t> buffer_vec_to_gnb(N);

//     char buffer_acc[10];
//     char buffer[BUFFER_MAX];
//     int size = 0;
//     int size_ue_2 = 0;
//     int size_recv = 0;
//     int size_from_ue_tx = 0;

//     int num_ues = 1;
//     int num_gnbs = 1;
//     int broker_rcv_accept_ues[num_ues];
//     int broker_rcv_accept_gnbs[num_gnbs];
//     int tx_data_count = 0;
//     int tx_count_lim = 0;
//     float pl_ue_2 = 10.0f;

//     int nbytes = buffer_vec.size() * sizeof(cf_t);
//     int received_from_matlab = 0;
//     int summ = 0;
//     while(1){
//         // receive conn accept from RX's
        
//         memset(buffer_acc, 0, sizeof(buffer_acc));
//         size = zmq_recv(send_socket_for_gnb_rx, buffer_acc, sizeof(buffer_acc), 0);
//         if(size == -1){
//             printf("!!!!!!!!!!!   -----1 send_socket_for_gnb_rx\n");
//             broker_rcv_accept_gnbs[0] = 0;
//             break;
//         } else{
//             broker_rcv_accept_gnbs[0] = 1;
//             printf("broker received [buffer_acc] form GNB RX = %d\n", size);
//         }


//         memset(buffer_acc, 0, sizeof(buffer_acc));
//         size = zmq_recv(send_socket_for_ue_1_rx, buffer_acc, sizeof(buffer_acc), 0);
//         if(size == -1){
//             printf("!!!!!!!!!!!   -----1 send_socket_for_ue_1_rx\n");
//             broker_rcv_accept_ues[0] = 0;
//             break;
//         } else{
//             broker_rcv_accept_ues[0] = 1;
//             printf("broker received [buffer_acc] from UE[1] RX = %d\n", size);
//         }
//         for (int i = 0; i < num_ues; i++){
//             summ += broker_rcv_accept_ues[i];
//         }

//         if (summ >= 1  && broker_rcv_accept_gnbs[0] == num_gnbs)
//         {
//             int send = 0;
            
//             // send accepts to TX's
//             send = zmq_send(req_socket_from_gnb_tx, buffer_acc, size, 0);
//             printf("req_socket_from_gnb_tx [send] = %d\n", send);
//             send = zmq_send(req_socket_from_ue_1_tx, buffer_acc, size, 0);
//             printf("req_socket_from_ue_1_tx [send] = %d\n", send);



//             // start data transmissiona
//             std::fill(buffer_vec.begin(), buffer_vec.end(), 0);
//             size = zmq_recv(req_socket_from_gnb_tx,  (void*)buffer_vec.data(), nbytes, 0);
//             if (size != -1)
//             {
//                 printf("broker received from gNb =  %d size packet buffer size = %d\n", size,buffer_vec.size());
//             }
//             send = zmq_send(send_socket_for_ue_1_rx, (void*)buffer_vec.data(), size, 0);
//             printf("send_socket_for_ue_1_rx [send data] = %d\n", send);

//             // TODO: Concatenate samples
//             std::fill(buffer_vec.begin(), buffer_vec.end(), 0);
//             size = zmq_recv(req_socket_from_ue_1_tx, (void*)buffer_vec.data(), nbytes, 0);
//             if (size != -1)
//             {
//                 printf("broker [received data] from UE[1] %d size packet\n", size);
//             }
//             buffer_vec.insert(buffer_vec.begin(), 111); // Inserts '1' at the beginning
//             size += sizeof(cf_t);

//             // send to matlab
//             send = zmq_send(matlab_server_req_socket, (void*)buffer_vec.data(), size, 0);
//             printf("send to matlab from UE1 %d size packet\n", size);

//             std::fill(buffer_vec.begin(), buffer_vec.end(), 0);
//             size = zmq_recv(matlab_server_req_socket, (void*)buffer_vec.data(), nbytes, 0);

//             // send to matlab
//             send = zmq_send(matlab_server_req_socket, (void*)buffer_vec.data(), size, 0);
//             printf("send to matlab from UE1 %d size packet\n", size);

//             std::fill(buffer_vec.begin(), buffer_vec.end(), 0);
//             size = zmq_recv(matlab_server_req_socket, (void*)buffer_vec.data(), nbytes, 0);

//             printf("received from matlab %d size packet\n", size);
//             int max_size = size;
//             send = zmq_send(send_socket_for_gnb_rx, (void*)buffer_vec.data(), max_size, 0);
//             printf("send_socket_for_gnb_rx [send data] = %d\n", send);
//             sleep(100);
//             tx_data_count++;
//             summ = 0;
//         }
//         else
//         {
//             continue;
//         }
//     }

//     zmq_close(send_socket_for_gnb_rx);
//     zmq_close (req_socket_from_ue_1_tx);

//     return 0;
// }
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

int main(){

    using cf_t = std::complex<float>;
    int N = 10;
    std::vector<std::complex<float>> buffer_vec(N);
    int nbytes = buffer_vec.size() * sizeof(std::complex<float>);

    int matlab_port = 4001;
    std::string matlab_server = "tcp://localhost:" + std::to_string(matlab_port);

    void *context = zmq_ctx_new ();
    void *matlab_server_req_socket = zmq_socket (context, ZMQ_REQ);
    int ret = zmq_connect(matlab_server_req_socket, matlab_server.c_str());
    printf("Connection to matlab server ret = %d\n",ret);

    for (int i = 0; i < buffer_vec.size(); i++){
        std::complex<float> val_1;
        val_1 = std::complex<float>(i*0.1f, i*0.2f);
        buffer_vec[i] = val_1;
        std::cout << "vect = " << buffer_vec[i] << std::endl;
    }

    for (int i = 0; i < 6; i++){

        buffer_vec[0] = std::complex<float>(10*i, 20);

        // send to matlab
        int send = zmq_send(matlab_server_req_socket, (void*)buffer_vec.data(), nbytes, 0);
        printf("send to matlab from UE1 %d size packet\n", nbytes);

        std::fill(buffer_vec.begin(), buffer_vec.end(), 0);
        int size = zmq_recv(matlab_server_req_socket, (void *)buffer_vec.data(), nbytes, 0);


        for (int i = 0; i < N; i++){
            std::cout << "vect = " << buffer_vec[i] << std::endl;
        }
    }

    buffer_vec[0] = std::complex<float>(255, 20);
    int send = zmq_send(matlab_server_req_socket, (void*)buffer_vec.data(), nbytes, 0);
    printf("send to matlab from UE1 %d size packet\n", nbytes);

    std::fill(buffer_vec.begin(), buffer_vec.end(), 0);
    int size = zmq_recv(matlab_server_req_socket, (void *)buffer_vec.data(), nbytes, 0);

    for (int i = 0; i < N; i++){
            std::cout << "vect = " << buffer_vec[i] << std::endl;
    }
    return 0;
}
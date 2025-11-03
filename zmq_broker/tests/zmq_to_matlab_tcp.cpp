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

    int N = 10;
    std::vector<int> buffer_vec(N);
    int nbytes = buffer_vec.size() * sizeof(int);

    int matlab_port = 4001;
    std::string matlab_server = "tcp://localhost:" + std::to_string(matlab_port);

    void *context = zmq_ctx_new ();
    void *matlab_server_req_socket = zmq_socket (context, ZMQ_ROUTER);
    int ret = zmq_connect(matlab_server_req_socket, matlab_server.c_str());
    printf("Connection to matlab server ret = %d\n",ret);

    for (int i = 0; i < buffer_vec.size(); i++){
        buffer_vec[i] = i;
        std::cout << "vect = " << buffer_vec[i] << std::endl;
    }
    // send to matlab
    int send = zmq_send(matlab_server_req_socket, (void*)buffer_vec.data(), nbytes, 0);
    printf("send to matlab from UE1 %d size packet\n", nbytes);

    std::fill(buffer_vec.begin(), buffer_vec.end(), 0);
    int size = zmq_recv(matlab_server_req_socket, (void *)buffer_vec.data(), nbytes, 0);

    for (int i = 0; i < N; i++){
        std::cout << "vect = " << buffer_vec[i] << std::endl;
    }
    return 0;
}

// #include <arpa/inet.h> // inet_addr()
// #include <netdb.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <strings.h> // bzero()
// #include <sys/socket.h>
// #include <unistd.h> // read(), write(), close()
// #define MAX 80
// #define PORT 4004
// #define SA struct sockaddr

// void func(int sockfd)
// {
//     int N = 10;
//     std::vector<int> buffer_vec(N);
//     for (int i = 0; i < buffer_vec.size(); i++){
//         buffer_vec[i] = i;
//         std::cout << "vect = " << buffer_vec[i] << std::endl;
//     }
//     int nbytes = buffer_vec.size() * sizeof(int);
//     for (int i = 0; i < 1; i++) {
//         std::fill(buffer_vec.begin(), buffer_vec.end(), 0);

//         //send(sockfd, &nbytes, sizeof(nbytes), 0);
//         send(sockfd, buffer_vec.data(), buffer_vec.size() * sizeof(int), 0);

//         std::fill(buffer_vec.begin(), buffer_vec.end(), 0);
//         uint32_t num_elements;
//         //recv(sockfd, &num_elements, sizeof(num_elements), 0);
//         std::vector<int> received_data(num_elements);
//         recv(sockfd, received_data.data(), num_elements * sizeof(int), 0);

//         for (int i = 0; i < received_data.size(); i++){
//             std::cout << "recv = " << received_data[i] << std::endl;
//         }
//     }
// }

// int main()
// {
//     int sockfd, connfd;
//     struct sockaddr_in servaddr, cli;

//     // socket create and verification
//     sockfd = socket(AF_INET, SOCK_STREAM, 0);
//     if (sockfd == -1) {
//         printf("socket creation failed...\n");
//         exit(0);
//     }
//     else
//         printf("Socket successfully created..\n");
//     bzero(&servaddr, sizeof(servaddr));

//     // assign IP, PORT
//     servaddr.sin_family = AF_INET;
//     servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
//     servaddr.sin_port = htons(PORT);

//     // connect the client socket to server socket
//     if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))
//         != 0) {
//         printf("connection with the server failed...\n");
//         exit(0);
//     }
//     else
//         printf("connected to the server..\n");

//     // function for chat
//     func(sockfd);

//     // close the socket
//     close(sockfd);
// }
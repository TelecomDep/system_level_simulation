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
#include <string.h>


int main(int argc, char *argv[]){

    //create socket
    void *context = zmq_ctx_new ();
    void *socket = zmq_socket (context, ZMQ_STREAM);

    if(zmq_bind(socket, "tcp://*:4000")){
        perror("zmq_bind");
        return 1;
    } else {
        printf("zmq_bind success\n");
    }

    //get client id 
    zmq_msg_t id_msg;
    zmq_msg_init(&id_msg);
    zmq_msg_recv(&id_msg, socket, 0);

    size_t id_size = zmq_msg_size(&id_msg);
    char *id_data = new char[id_size];
    memcpy(id_data, zmq_msg_data(&id_msg), id_size);

    printf("Client connected\n");

    zmq_msg_close(&id_msg);


    char *recv_buffer;
    const char *data = "msg to client";
    int size = 0;

    zmq_msg_t recv_id;
    zmq_msg_t recv_data;
    size_t recv_size;

    zmq_msg_t new_id_msg;


    int ue_num = 0;

    while(true){

        //recv

        //recv data
        zmq_msg_init(&recv_data);
        zmq_msg_recv(&recv_data, socket, 0);

        //recv id
        zmq_msg_init(&recv_id);
        zmq_msg_recv(&recv_id, socket, 0);
          
        recv_size = zmq_msg_size(&recv_data);
        
        if (recv_size > 0) {

            recv_buffer = new char[recv_size];

            memcpy(recv_buffer, zmq_msg_data(&recv_data), recv_size);

            printf("Received from client: %s\n", recv_buffer);

            delete[] recv_buffer;
        }
        
        zmq_msg_close(&recv_id);
        zmq_msg_close(&recv_data);

        usleep(100);

        //send data

        zmq_msg_init_size(&new_id_msg, id_size);
        memcpy(zmq_msg_data(&new_id_msg), id_data, id_size);

        //send ID
        zmq_msg_send(&new_id_msg, socket, ZMQ_SNDMORE);

        //send data
        std::vector<uint8_t> send_data(strlen(data));
        send_data[0] = static_cast<uint8_t>(ue_num);
        memcpy(send_data.data() + 1, data, strlen(data));

        size = zmq_send(socket, send_data.data(), sizeof(send_data), 0);

        if(size == -1){
            printf("error");
        } else{
            printf("Send to client: %s \n", data);
        }

        ue_num++;
    }

    delete[] id_data;
    zmq_msg_close(&id_msg);
    zmq_close(socket);

}
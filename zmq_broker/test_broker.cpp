#include <zmq.h>
#include <vector>
#include <complex>
#include <cstring>

int main(){

    //create data
    std::vector<std::complex<float>> samples;
    samples.resize(11000);

    for(int i = 0; i < samples.size(); ++i){
        samples[i] = {rand() % 10000, rand() % 10000};
    }

    //create context and socket
    void *context = zmq_ctx_new();
    
    void *socket_to_matlab = zmq_socket (context, ZMQ_STREAM);
    
    if(zmq_bind(socket_to_matlab, "tcp://*:4000")){
        perror("zmq_bind");
        return 1;
    } else {
        printf("zmq_bind success\n");
    }

    //sub vars
    bool matlab_connected = false;

    zmq_msg_t id_msg;

    size_t id_size;
    char *id_data;

    int size;

    std::vector<std::complex<float>> buffer(samples.size());

    printf("WAIT MATLAB\n");

    while (true){
        //wait matlab
        if(!matlab_connected){

            zmq_msg_init(&id_msg);
            zmq_msg_recv(&id_msg, socket_to_matlab, 0);

            id_size = zmq_msg_size(&id_msg);
            id_data = new char[id_size];

            memcpy(id_data, zmq_msg_data(&id_msg), id_size);

            if(id_size > 0){
                printf("\nMatlab connected successfully\n");
                matlab_connected = true;
            } 
            
            else{
                continue;
            }

            zmq_msg_close(&id_msg);
        }

        //start data tx/rx

        else{
    
            size = zmq_send(socket_to_matlab, (void*)samples.data(), samples.size() * sizeof(std::complex<float>), 0);
    
            printf("BROKER SEND TO MATLAB %d BYTE\n", size);
    
            size = zmq_recv(socket_to_matlab, (void*)buffer.data(), size, 0);
    
            printf("BROKER RECIEVE FROM MATLAB %d BYTE\n", size);
        }


    }


    return 0;
}
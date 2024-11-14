#include <iostream>
#include <mutex>
#include <zmq.h>
#include <thread>
#include <cstring>

#define BUFFER_MAX 1024 * 1024

static bool run;
std::mutex mutex_send_matlab;

enum class ARGV_CONSOLE {
    ARGV_PORT_1 = 1,
    ARGV_PORT_2,
    ARGV_PORT_1_PROXY,
    ARGV_PORT_2_PROXY,
    ARGV_PORT_API,
    ARGV_MAX
};

void send_to_matlab(char *data, int size, void *socket_api) {
    static char buffer[1000];
    mutex_send_matlab.lock();
    zmq_send(socket_api, data, size, 0);
    zmq_recv(socket_api, buffer, sizeof(buffer), 0);
    printf("msg from matlab: %s\n", buffer);
    mutex_send_matlab.unlock();
}   

void thread_proxy(void *zrecv, void *zsend, void *socket_api, int id) {
    
    char buffer[BUFFER_MAX];
    int size;
    printf("[%d] start\n", id);
    while(1) {
        memset(buffer, 0, sizeof(buffer));
        size = zmq_recv(zrecv, buffer, sizeof(buffer), 0);
        
        if(size == -1) {
            continue;
        }
        printf("[%d] send request[%d]...\n", id, size);
        zmq_send(zsend, buffer, size, 0);
        send_to_matlab(buffer, size, socket_api);
        printf("[%d] end iter\n", id);
    }
}

void thread_proxy_2(void *zrecv, void *zsend, void *socket_api, int id) {
    
    char buffer[BUFFER_MAX];
    int size;
    printf("[%d] start\n", id);
    while(1) {
        memset(buffer, 0, sizeof(buffer));
        size = zmq_recv(zsend, buffer, sizeof(buffer), 0);
        if(size == -1) {
            continue;
        }
        printf("[%d] send response[%d]...\n", id, size);
        zmq_send(zrecv, buffer, size, 0);
        send_to_matlab(buffer, size, socket_api);
        printf("[%d] end iter\n", id);

    }
}

int main(int argc, char *argv[]){

    if(argc < static_cast<int>(ARGV_CONSOLE::ARGV_MAX) - 1) {
        printf("Error: not found argv\n");
        return -1;
    }
    int port1 = std::stoi(argv[static_cast<int>(ARGV_CONSOLE::ARGV_PORT_1)]);
    int port2 = std::stoi(argv[static_cast<int>(ARGV_CONSOLE::ARGV_PORT_2)]);
    int port1_proxy = std::stoi(argv[static_cast<int>(ARGV_CONSOLE::ARGV_PORT_1_PROXY)]);
    int port2_proxy = std::stoi(argv[static_cast<int>(ARGV_CONSOLE::ARGV_PORT_2_PROXY)]);
    int port_api = std::stoi(argv[static_cast<int>(ARGV_CONSOLE::ARGV_PORT_API)]);
    
    void *context = zmq_ctx_new ();
    void *requester = zmq_socket (context, ZMQ_REQ);
    void *requester2 = zmq_socket (context, ZMQ_REQ);
    void *requester_api = zmq_socket (context, ZMQ_REQ);
    printf("%d %d %d %d\n", port1, port2, port1_proxy, port2_proxy);
    std::string addr_recv_1 = "tcp://localhost:" + std::to_string(port1);
    std::string addr_recv_2 = "tcp://localhost:" + std::to_string(port2);
    std::string addr_send_1 = "tcp://*:" + std::to_string(port1_proxy);
    std::string addr_send_2 = "tcp://*:" + std::to_string(port2_proxy);
    std::string addr_socket_api = "tcp://localhost:" + std::to_string(port_api);
    printf("connecting to %s...\n", addr_recv_1.c_str());
    zmq_connect (requester, addr_recv_1.c_str());
    printf("connecting to %s...\n", addr_recv_2.c_str());
    zmq_connect (requester2, addr_recv_2.c_str());
    printf("connecting to %s...\n", addr_socket_api.c_str());
    zmq_connect (requester_api, addr_socket_api.c_str());
    void *socket_proxy1 = zmq_socket(context, ZMQ_REP);
    void *socket_proxy2 = zmq_socket(context, ZMQ_REP);
    if(!socket_proxy1) {
        perror("zmq_socket");
        return 1;
    }
    if(!socket_proxy2) {
        perror("zmq_socket");
        return 1;
    }
    printf("[Init]\n");
    if(zmq_bind(socket_proxy1, addr_send_1.c_str()) != 0) {
        perror("zmq_bind");
        return 1;
    }
    if(zmq_bind(socket_proxy2, addr_send_2.c_str())) {
        perror("zmq_bind");
        return 1;
    }
    std::thread thr1;
    std::thread thr2;
    std::thread thr3;
    std::thread thr4;
    thr1 = std::thread(thread_proxy, socket_proxy1, requester, requester_api, 1);
    thr1.detach();
    thr3 = std::thread(thread_proxy_2, socket_proxy1, requester, requester_api, 2);
    thr3.detach();
    thr2 = std::thread(thread_proxy, socket_proxy2, requester2, requester_api, 3);
    thr2.detach();
    thr4 = std::thread(thread_proxy_2, socket_proxy2, requester2, requester_api, 4);
    thr4.detach();
    while(1) {
        sleep(1);
    }
    printf("End client\n");
    zmq_close (requester);
    zmq_close (requester2);
    zmq_close (socket_proxy1);
    zmq_close (socket_proxy2);
    zmq_close (requester_api);
    zmq_ctx_destroy (context);
    printf("[Clear]\n");
    return 0;
}









































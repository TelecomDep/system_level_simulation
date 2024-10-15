#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>


#include <time.h>
#include <pcap.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

#define ETH_HEADER_SIZE 14

typedef unsigned char u_char;

void print_packet_info(const u_char *packet){
    struct ip *ip_header = (struct ip *)(packet + ETH_HEADER_SIZE);
    u_char *payload = packet + ETH_HEADER_SIZE + (ip_header->ip_hl * 4);
    char source_ip[INET_ADDRSTRLEN];
    char dest_ip[INET_ADDRSTRLEN];
    // int c_port = ntohs(ip_header.sin_port);
    inet_ntop(AF_INET, &(ip_header->ip_src), source_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(ip_header->ip_dst), dest_ip, INET_ADDRSTRLEN);
    
    printf("source ip: %s\n", source_ip);
    printf("dest ip: %s\n", dest_ip);
    // printf("source port: %d\n", );
    // printf("dest port: %d\n");
    
    printf("size ip header: %d\n", ip_header->ip_hl * 4);
    printf("size total: %d\n", ip_header->ip_len);
    printf("payload:\n");
    for(int i = 0; i < ip_header->ip_len - (ip_header->ip_hl * 4); ++i) {
        printf("%x", (u_char)*(payload + i));
        if(i != 0 && i % 40 == 0) {
            printf("\n");
            if(i > 1000) {
                break;
            }
        }
    }
    printf("\n");

    // printf("payload: ", ip_header->ip_hl);
    
    // packet_header
    printf("---\n");
}

void packet_handler(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {
    // Здесь вы можете обрабатывать пакет и отправлять его через ZMQ
    // Пример: отправка пакета по ZeroMQ
    // zmq_msg_t zmq_message;
    // zmq_msg_init_size(&zmq_message, header->len);
    // memcpy(zmq_msg_data(&zmq_message), packet, header->len);

    // // Отправка сообщения (нужно предварительно создать сокет)
    // zmq_msg_send(&zmq_message, (zmq_socket_t *)args, 0);
    // zmq_msg_close(&zmq_message);
    printf("dest ip\n");
}

int part_1(size_t packet_len, const u_char *packet) {
    struct pcap_pkthdr hdr;
    hdr.ts.tv_sec = time(NULL); // timestamp (seconds)
    hdr.ts.tv_usec = 0; // timestamp (microseconds)
    hdr.caplen = packet_len; // length of the packet data
    hdr.len = packet_len; // length of the packet data
    print_packet_info(packet);
}

#define CONDITION 2

#if CONDITION == 1

int main (void)
{
    //  Socket to talk to clients
    void *context = zmq_ctx_new ();
    void *responder = zmq_socket (context, ZMQ_REP);
    int rc = zmq_bind (responder, "tcp://*:2001");
    assert (rc == 0);


    char *device;
    char error_buffer[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
    pcap_if_t *devs;
    const u_char *packet;
     struct pcap_pkthdr packet_header;
    int packet_count_limit = 10;
    int timeout_limit = 10000; /* In milliseconds */

#if 1
  printf("%s:%d\n", __func__, __LINE__);

    pcap_findalldevs(&devs, error_buffer);
    if (devs == NULL) {
        printf("Error finding device: %s\n", error_buffer);
        return 1;
    }

    /* Open device for live capture */
    handle = pcap_open_live(
            devs->name,
            BUFSIZ,
            packet_count_limit,
            timeout_limit,
            error_buffer
        );
 printf("%s:%d\n", __func__, __LINE__);
     /* Attempt to capture one packet. If there is no network traffic
      and the timeout is reached, it will return NULL */
#if 0
     packet = pcap_next(handle, &packet_header);
     if (packet == NULL) {
        printf("No packet found.\n");
        return 2;
    }
#endif
#endif
    /* Our function to output some info */
    
    printf("%s:%d\n", __func__, __LINE__);

    while (1) {
        char buffer [1024];
        // size_t packet_len = zmq_recv (responder, buffer, sizeof(buffer), 0);
        // part_1(packet_len, (u_char*)buffer);


        pcap_loop(handle, 0, packet_handler, (u_char *)responder);
        
        
        // zmq_msg_recv()
        printf("%s:%d\n", __func__, __LINE__);


        // print_packet_info(packet, packet_header);
        printf ("Received Hello\n");
        sleep (1);          //  Do some 'work'
        zmq_send (responder, "World", 5, 0);
    }
    return 0;
}
#endif

#if CONDITION == 2


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <zmq.h>



void packet_handler2(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {
    // Привязка аргументов к сокету ZMQ
    void *socket = args;
    printf("%s:%d\n", __func__, __LINE__);

    print_packet_info(packet);
    // Создание сообщения ZMQ
    // zmq_msg_t zmq_message;
    // zmq_msg_init_size(&zmq_message, header->len);
    // memcpy(zmq_msg_data(&zmq_message), packet, header->len);

    // // Отправка сообщения через сокет
    // zmq_msg_send(&zmq_message, socket, 0);
    // zmq_msg_close(&zmq_message);
}

int main() {

    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, ZMQ_PUSH);
    zmq_bind(socket, "tcp://*:2001");
    printf("%s:%d\n", __func__, __LINE__);

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *alldevs, *device;
    
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        fprintf(stderr, "Couldn't find devices: %s\n", errbuf);
        return 1;
    }
    printf("Доступные интерфейсы:\n");
    for (device = alldevs; device; device = device->next) {
        if(strlen("lo") == strlen(device->name) && !strcmp(device->name, "lo")) {
            break;
        }
        // printf("%s\n", device->name);
    }
    // device = alldevs;
    if (device == NULL) {
        fprintf(stderr, "No devices found.\n");
        return 1;
    }
    pcap_t *handle = pcap_open_live(device->name, BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Could not open device %s: %s\n", device->name, errbuf);
        pcap_freealldevs(alldevs);
        return 1;
    }
    printf("%s:%d\n", __func__, __LINE__);

    int result = pcap_loop(handle, 0, packet_handler2, (u_char *)socket);
    printf("%s:%d\n", __func__, __LINE__);
    pcap_close(handle);
    zmq_close(socket);
    zmq_ctx_destroy(context);
    pcap_freealldevs(alldevs);

    return (result == -1) ? 1 : 0;
}


#endif
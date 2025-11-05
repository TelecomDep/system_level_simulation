#include "../includes/Broker.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;


Broker::Broker(std::string &config_file, std::vector<Equipment>& _ues, std::vector<Equipment>& _gnbs)
{
    std::ifstream f(config_file);
    json data = json::parse(f);
    ues = _ues;
    gnbs = _gnbs;
    concatenate_to_gnb_samples = std::vector<std::complex<float>>(buff_size);
    matlab_samples = std::vector<std::complex<float>>(buff_size);

    // TODO: fix nullptr while auto
    // for (const auto& item : data["ues"])
    // {
    //     ues.push_back(Equipment(item["rx_port"], item["tx_port"], item["id"], item["type"]));
    // }

    // for (const auto& item : data["gnbs"])
    // {
    //     gnbs.push_back(Equipment(item["rx_port"], item["tx_port"], item["id"], item["type"]));
    // }


    matlab_port = data["matlab"]["server_port"];
    std::cout << "matlab port = " << matlab_port << std::endl;
}

Broker::Broker(){};

std::vector<std::complex<float>> Broker::byte_to_complex(const std::vector<uint8_t>& buffer) {
    
    if (buffer.size() < sizeof(std::complex<float>) + 1) {
        std::cerr << "Error: buffer too small to contain any std::std::complex samples\n";
        return {};
    }

    size_t num_samples = (buffer.size()) / sizeof(std::complex<float>);

    if (num_samples > std::numeric_limits<size_t>::max() / sizeof(std::complex<float>)) {
        std::cerr << "Error: too many samples, would overflow\n";
        return {};
    }

    std::vector<std::complex<float>> result(num_samples);
    memcpy(result.data(), buffer.data() + 1, num_samples * sizeof(std::complex<float>));
    
    return result;
}


int Broker::send_to_matlab(void *socket_to_matlab, std::vector<uint8_t> &data, int data_size, char *id_data,size_t id_size){

    zmq_msg_t recv_id;
    zmq_msg_t recv_data;
    size_t recv_size;
    zmq_msg_t new_id_msg;

    int size = 0;

    zmq_msg_init_size(&new_id_msg, id_size);
    memcpy(zmq_msg_data(&new_id_msg), id_data, id_size);

    //send ID
    zmq_msg_send(&new_id_msg, socket_to_matlab, ZMQ_SNDMORE);

    zmq_msg_close(&new_id_msg);

    size = zmq_send(socket_to_matlab, data.data(), data_size, 0);

    return size;
}

bool Broker::send_from_ues_to_matalb_and_send_to_gnb()
{

    int gnb_id = gnbs[0].getId();
    int ue_id = 0;
    int N = 10;
    std::vector<int> buffer_acc(N);
    std::complex<float> ids;
    int max_size = 0;
    std::fill(concatenate_to_gnb_samples.begin(), concatenate_to_gnb_samples.end(), 0);
    for (int i = 0; i < ues.size(); i++)
    {
        if(ues[i].is_active){
            ue_id = ues[i].getId();
            ids = std::complex<float>((float)ue_id, (float)gnb_id);
            matlab_samples = ues[i].samples_to_transmit;
            matlab_samples.insert(matlab_samples.begin(), ids);

            int send = zmq_send(matlab_req_socket, (void *)matlab_samples.data(), ues[i].get_recv_nbytes() + sizeof(std::complex<float>), 0);
            if(send == -1){
                printf("Error sending into matlab\n");
            } else{
                printf("send to matlab from ues size packet = %d\n", send);
            }
            matlab_samples.erase(matlab_samples.begin());

            std::fill(buffer_acc.begin(), buffer_acc.end(), 0);
            int size = zmq_recv(matlab_req_socket, (void *)buffer_acc.data(), N, 0);
            if(size == -1){
                printf("Error recv from matlab\n");
            } else {
                printf("revc from matlab accept = %d\n", size);
            }


            if(max_size < ues[i].get_recv_nbytes() - sizeof(std::complex<float>)){
                max_size = ues[i].get_recv_nbytes();
                printf("max_size = %d\n", max_size );
            }
        }
    }

    matlab_samples.insert(matlab_samples.begin(), std::complex<float>(255, 255));
    int send = zmq_send(matlab_req_socket, (void*)matlab_samples.data(), max_size + sizeof(std::complex<float>), 0);
    printf("send to matlab from UE1 %d size packet\n", max_size + sizeof(std::complex<float>));
    matlab_samples.erase(matlab_samples.begin());

    concatenate_to_gnb_samples.insert(concatenate_to_gnb_samples.begin(), ids);
    //std::fill(concatenate_to_gnb_samples.begin(), concatenate_to_gnb_samples.end(), 0);
    int size = zmq_recv(matlab_req_socket, (void *)concatenate_to_gnb_samples.data(), max_size + sizeof(std::complex<float>), 0);
    printf("recv from matlab concatenated samples = %d\n", size);
    concatenate_to_gnb_samples.erase(concatenate_to_gnb_samples.begin());

    gnbs[0].send_samples_to_rx(concatenate_to_gnb_samples, size - sizeof(std::complex<float>));

    
    // std::fill(concatenate_to_gnb_samples.begin(), concatenate_to_gnb_samples.end(), 0);
    // std::fill(matlab_samples.begin(), matlab_samples.end(), 0);
    return true;
}

bool Broker::send_from_gnb_to_matlab_per_ue()
{
    int gnb_id = gnbs[0].getId();
    int ue_id = 0;
    int N = 10;
    std::vector<int> buffer_acc(N);
    std::complex<float> ids;
    for (int i = 0; i < ues.size(); i++)
    {
        ue_id = ues[i].getId();
        std::cout << "gnbid = " << gnb_id << std::endl;
        matlab_samples = gnbs[0].samples_to_transmit;
        ids = std::complex<float>((float)gnb_id, (float)ue_id);
        matlab_samples.insert(matlab_samples.begin(), ids);

        int send = zmq_send(matlab_req_socket, (void *)matlab_samples.data(), gnbs[0].get_recv_nbytes() + sizeof(std::complex<float>), 0);
        if(send == -1){
            printf("Error sending into matlab\n");
        } else{
            printf("send to matlab from gNb size packet = %d\n", send);
        }
        

        std::fill(matlab_samples.begin(), matlab_samples.end(), 0);
        int size = zmq_recv(matlab_req_socket, (void *)matlab_samples.data(), gnbs[0].get_recv_nbytes() + sizeof(std::complex<float>), 0);
        if(size == -1){
            printf("Error recv from matlab\n");
        } else {
            printf("revc from matlab accept = %d\n", size);
        }
        matlab_samples.erase(matlab_samples.begin());
    
        ues[i].send_samples_to_rx(matlab_samples, gnbs[0].get_recv_nbytes());
        
    }
    //nbytes_form_gnb = 0;
    return true;
}


void Broker::initialize_zmq_sockets()
{
    zmq_context = zmq_ctx_new();

    std::string matlab_server = "tcp://localhost:" + std::to_string(matlab_port);
    matlab_req_socket = zmq_socket (zmq_context, ZMQ_REQ);
    int ret = zmq_connect(matlab_req_socket, matlab_server.c_str());
    if(ret < 0){
        printf("Failed to connect to Matlab\n");
        is_matlab_connected = false;
    }
    else
    {
        is_matlab_connected = true;
        printf("CONNECTED to Matlab\n");
    }

    gnbs[0].initialize_sockets(zmq_context);
    gnbs[0].activate();
    for (int i = 0; i < ues.size(); i++)
    {
        ues[i].initialize_sockets(zmq_context);
        ues[i].activate();
    }
}

bool Broker::recv_conn_accepts()
{
    broker_acc_count = 0;

    gnbs[0].recv_conn_accept();
    broker_acc_count += gnbs[0].is_ready_to_send();

    for(int i = 0; i < ues.size();i++)
    {
        ues[i].recv_conn_accept();
        broker_acc_count += ues[i].is_ready_to_send();
    }

    if(broker_acc_count >= 2){
        return true;
    } else {
        printf("Not all connection Accepts are received\n");
        return false;
    }
}

bool Broker::send_conn_accepts()
{
    gnbs[0].send_conn_accept();
    
    for(int i = 0; i < ues.size();i++)
    {
        ues[i].send_conn_accept();
    }
    return true;
}

void Broker::send_recv_samples_from_Ues_to_matlab()
{

}

bool Broker::recv_samples_from_gNb()
{
    // TODO: Пока работает только с 1 базово станцией
    nbytes_form_gnb = gnbs[0].recv_samples_from_tx(buff_size);

    return true;
}

bool Broker::send_samples_to_all_ues()
{
    for(int i = 0; i < ues.size();i++)
    {
        ues[i].send_samples_to_rx(gnbs[0].get_samples_tx(), nbytes_form_gnb);
    }
    return true;
}

bool Broker::recv_samples_from_ues()
{
    for (int i = 0; i < ues.size();i++)
    {
        ues[i].recv_samples_from_tx(buff_size);
    }

    return true;
}

bool Broker::send_samples_to_gnb()
{
    // TODO: добавить передачу сэмплов, полученных с матлаба
    int max_size = 0;
    std::fill(concatenate_to_gnb_samples.begin(), concatenate_to_gnb_samples.end(), 0);
    for (int i = 0; i < ues.size(); i++)
    {
        ues[i].divide_samples_by_value((i+1)*10.0f); // path losses
        // TODO: либо concatenate
        //concatenate_to_gnb_samples
        
        if(ues[i].is_active){
            if(max_size < ues[i].get_nbytes_recv_from_tx())
            {
                max_size = ues[i].get_nbytes_recv_from_tx();
            }
            std::transform( concatenate_to_gnb_samples.begin(), 
                            concatenate_to_gnb_samples.end(), 
                            ues[i].get_samples_tx().begin(), 
                            concatenate_to_gnb_samples.begin(), 
                            std::plus<std::complex<float>>());
        }
    }
    
    gnbs[0].send_samples_to_rx(concatenate_to_gnb_samples, nbytes_form_gnb);
    return true;
}


void Broker::run_the_world()
{
    broker_working_counter = 0;
    while (is_running)
    {
        if(is_matlab_connected){
            std::cout << "---------------------------------------" << std::endl;
            if (recv_conn_accepts())
            {
                std::cout << "------------->>send_conn_accepts()" << std::endl;
                send_conn_accepts();

                std::cout << "------------->>recv_samples_from_gNb()" << std::endl;
                recv_samples_from_gNb();
                send_from_gnb_to_matlab_per_ue();

                std::cout << "------------->>recv_samples_from_ues()" << std::endl;
                recv_samples_from_ues();

                send_from_ues_to_matalb_and_send_to_gnb();
            } else {
                continue;
            }
        } else {
            if (recv_conn_accepts())
            {
                send_conn_accepts();
                recv_samples_from_gNb();
                send_samples_to_all_ues();
                recv_samples_from_ues();
                send_samples_to_gnb();
            } else {
                continue;
            }

        }
        broker_working_counter++;
    }
}

void Broker::start_the_proxy()
{
    initialize_zmq_sockets();
    run_the_world();
}

// std::vector<int> Broker::get_tx_samples_sizes() const{
//     std::vector<int> result;

//     for(const Equipment& el : gnbs){
//         result.push_back(el.get_samples_tx().size());
//     }

//     for(const Equipment& el : ues){
//         result.push_back(el.get_samples_tx().size());
//     }

//     return result;
// }

// std::vector<int> Broker::get_rx_samples_sizes() const{
//     std::vector<int> result;

//     for(const Equipment& el : ues){
//         result.push_back(el.get_samples_rx().size());
//     }

//     return result;
// }

// //get tx samples from ues and gnbs
// std::vector<std::vector<std::complex<float>>> Broker::get_all_tx_samples() const{

//     std::vector<std::vector<std::complex<float>>> result;
    
//     for(const Equipment& el: gnbs){
//         result.push_back(el.get_samples_tx());
//     }

//     for(const Equipment& el: ues){
//         result.push_back(el.get_samples_tx());
//     }

//     return result;
// }

// std::vector<uint8_t> Broker::concatenate_tx_samples(){

//     std::vector<std::vector<std::complex<float>>> all_samples = get_all_tx_samples();

//     int len = 0; //len samples in bytes

//     for(const std::vector<std::complex<float>>& el : all_samples){
//         len += el.size() + 1;                                           // +1 for ID
//     }

//     std::vector<uint8_t> result(len);

//     int offset = 0;

//     for(int i = 0; i < all_samples.size(); ++i){

//         result[offset] = static_cast<uint8_t>(i);

//         offset++;

//         memcpy(result.data() + offset, all_samples[i].data(), all_samples[i].size());
//         offset += all_samples[i].size();
//     }

//     return result;
// }

// void Broker::run(){

//     if(!(gnbs.size() > 0 && ues.size() > 0)){
//         printf("Broker dont have gnbs or ues list");
//         return;
//     }

//     int num_ues = ues.size();
//     int num_gnbs = gnbs.size();

//     bool matlab_enable = matlab_port == -1 ? false : true;

//     std::string addr_recv_port_gnb_tx = "tcp://localhost:" + std::to_string(gnbs[0].get_tx_port());
//     std::string addr_send_port_gnb_rx = "tcp://*:" + std::to_string(gnbs[0].get_rx_port());

//     //Address
//     std::vector<std::string> addr_recv_ports_ue_tx(num_ues);
//     std::vector<std::string> addr_send_ports_ue_rx(num_ues);


//     for(int i = 0; i < num_ues; i++){
//         addr_send_ports_ue_rx[i] = "tcp://*:" + std::to_string(ues[i].get_rx_port());
//         addr_recv_ports_ue_tx[i] = "tcp://localhost:" + std::to_string(ues[i].get_tx_port());
//     }

//     // Initialize ZMQ
//     void *context = zmq_ctx_new ();

//     void *req_socket_from_gnb_tx = zmq_socket(context, ZMQ_REQ);

//     std::vector<void*> req_sockets_from_ue_tx(num_ues);

//     for(int i = 0; i < num_ues; ++i){
//         req_sockets_from_ue_tx[i] = zmq_socket(context, ZMQ_REQ);
//     }

//     if(zmq_connect(req_socket_from_gnb_tx, addr_recv_port_gnb_tx.c_str())){
//         perror("zmq_connect\n");
//         return;
//     } else {
//         printf("zmq_connect [req_socket_for_gnb_tx] - success\n");
//     }

//     for(int i = 0; i < num_ues; ++i){
//         if(zmq_connect(req_sockets_from_ue_tx[i], addr_recv_ports_ue_tx[i].c_str())){
//             perror("zmq_connect\n");
//             return;
//         } else {
//             printf("zmq_connect [req_socket_for_UE%d_tx] - success\n", i+1);
//         }   
//     }

//     void *send_socket_for_gnb_rx = zmq_socket(context, ZMQ_REP);

//     std::vector<void*>send_sockets_for_ue_rx(num_ues);

//     for(int i = 0; i < num_ues; ++i){
//         send_sockets_for_ue_rx[i] = zmq_socket(context, ZMQ_REP);
//     }

//     if(zmq_bind(send_socket_for_gnb_rx, addr_send_port_gnb_rx.c_str())) {
//         perror("zmq_bind\n");
//         return;
//     } else {
//         printf("zmq_bind [send_socket_for_gnb_rx]- success\n");
//     }

//     for(int i = 0; i < num_ues; ++i){
//         if(zmq_bind(send_sockets_for_ue_rx[i], addr_send_ports_ue_rx[i].c_str())){
//             perror("zmq_connect\n");
//             return;
//         } else {
//             printf("zmq_connect [req_socket_for_UE%d_rx] - success\n", i+1);
//         }   
//     }

//     //socket to matlab

//     void *socket_to_matlab;

//     if(matlab_enable){

//         socket_to_matlab = zmq_socket (context, ZMQ_STREAM);
    
//         std::string matlab_address = "tcp://*:" + std::to_string(matlab_port);
    
//         if(zmq_bind(socket_to_matlab, matlab_address.data())){
//             perror("zmq_bind");
//             return;
//         } else {
//             printf("zmq_bind [matlab_socket] - success\n");
//         }
//     }

//     int timeout = 25000;

//     zmq_setsockopt(req_socket_from_gnb_tx, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

//     for(int i = 0; i < num_ues; ++i){
//         zmq_setsockopt(req_sockets_from_ue_tx[i], ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
//     }

//     using cf_t = std::complex<float>;

//     int N = 80000;

//     int nbytes = N * sizeof(cf_t);

//     std::vector<cf_t> buffer_vec(N);
//     std::vector<cf_t> concatenate_buffer(N);

//     char buffer_acc[10];
//     int broker_rcv_accept_ues[num_ues] = {0};
//     int broker_rcv_accept_gnbs[num_gnbs] = {0};

//     int size;

//     std::vector<uint8_t> concatenate_samples;

//     std::vector<uint8_t> byte_packet_size;

//     bool matlab_connected = false;

//     std::vector<uint8_t> matlab_buffer(buffer_vec.size() * sizeof(cf_t) + 1);

//     std::vector<int> packet_sizes;

//     zmq_msg_t id_msg;

//     size_t id_size;
//     char *id_data;


//     while(1){

//         //check matlab conection if he is enable

//         if(!matlab_connected && matlab_enable){

//             zmq_msg_init(&id_msg);
//             zmq_msg_recv(&id_msg, socket_to_matlab, 0);
    
//             id_size = zmq_msg_size(&id_msg);
//             id_data = new char[id_size];
    
//             memcpy(id_data, zmq_msg_data(&id_msg), id_size);
    
//             if(id_size > 0){
//                 printf("\nMatlab connected successfully\n");
//                 matlab_connected = true;
//             } 
            
//             else{
//                 continue;
//             }
    
//             zmq_msg_close(&id_msg);
//         }

//         //receive conn accept from RX's

//         memset(buffer_acc, 0, sizeof(buffer_acc));

//         size = zmq_recv(send_socket_for_gnb_rx, buffer_acc, sizeof(buffer_acc), 0);

//         if(size == -1){

//             printf("!!!!!!!!!!!   -----1 send_socket_for_gnb_rx\n");

//             printf(zmq_strerror(zmq_errno()));

//             continue;

//         } else{

//             broker_rcv_accept_gnbs[0] = 1;

//             printf("broker received [buffer_acc] form GNB RX = %d\n", size);
//         }

//         for(int i = 0; i < num_ues; ++i){

//             memset(buffer_acc, 0, sizeof(buffer_acc));

//             size = zmq_recv(send_sockets_for_ue_rx[i], buffer_acc, sizeof(buffer_acc), 0);

//             if(size == -1){

//                 printf("!!!!!!!!!!!   -----1 send_socket_for_ue_%d_rx\n", i+1);

//                 continue;

//             } else{
//                 broker_rcv_accept_ues[i] = 1;

//                 printf("broker received [buffer_acc] from UE[%d] RX = %d\n", i+1, size);
//             }
//         }
    

//         int summ = 0;

//         for (int i = 0; i < num_ues; i++){
//             summ += broker_rcv_accept_ues[i];
//         }

//         printf("Accept UES: %d\n", summ);

//         if (summ == num_ues && broker_rcv_accept_gnbs[0] == num_gnbs)
//         {
//             int send = 0;

//             // send accepts to TX's
//             send = zmq_send(req_socket_from_gnb_tx, buffer_acc, size, 0);

//             printf("req_socket_from_gnb_tx [send] = %d\n", send);

//             for(int i = 0; i < num_ues; ++i){
//                 send = zmq_send(req_sockets_from_ue_tx[i], buffer_acc, size, 0);
//                 printf("req_socket_from_ue_%d_tx [send] = %d\n", i+1 ,send);
//             }


//             // start data transmissiona

//             fill(buffer_vec.begin(), buffer_vec.end(), 0);

//             //recv from gnb
//             size = zmq_recv(req_socket_from_gnb_tx, (void*)buffer_vec.data(), nbytes, 0);

//             if (size == -1){
//                 printf("ERROR: broker received from gNb = %d bytes\n", size);
//                 continue;
//             }

//             gnbs[0].set_samples_tx(buffer_vec, size);

//             printf("broker received from gNb %d bytes\n", size);

//             printf("\nsamples from gnb\n");
            
//             //check first 100 samples
//             for(int j = 0; j < 100; ++j){
//                 std::cout << gnbs[0].get_samples_tx()[j] << ", ";
//             }

//             printf("\n");

//             //recieve samples from ues
//             for(int i = 0; i < num_ues; ++i){

//                 fill(buffer_vec.begin(), buffer_vec.end(), 0);

//                 size = zmq_recv(req_sockets_from_ue_tx[i], (void*)buffer_vec.data(), nbytes, 0);

//                 if (size == -1){
//                     printf("ERROR: Broker recieved from UE%d %d bytes\n", i + 1, size);
//                     continue;
//                 }

//                 ues[i].set_samples_tx(buffer_vec, size);

//                 printf("\nbroker [received data] from UE[%d] %d size packet\n", i+1 ,size);
//             }

//             //send to MATLAB packet sizes

//             packet_sizes = get_tx_samples_sizes();

//             for(int el : packet_sizes){
//                 printf("%d ", el);
//             }

//             byte_packet_size = to_byte(packet_sizes);

//             size = send_to_matlab(socket_to_matlab, byte_packet_size, byte_packet_size.size(), id_data, id_size);

//             printf("\nsocket to matlab [send data] (packet size) = %d\n", size);

//             //concatenate ues and gnb samples

//             concatenate_samples = concatenate_tx_samples();

//             printf("concatenate samples size = %ld", concatenate_samples.size());

//             // //send all samples to MATLAB

//             size = send_to_matlab(socket_to_matlab, concatenate_samples, concatenate_samples.size(), id_data, id_size);

//             printf("\nsocket to matlab [send data] = %d\n", size);

//             // //receive all samples from matlab

//             matlab_buffer.resize(size);

//             size = receive_from_matlab(socket_to_matlab, matlab_buffer, size);

//             printf("\nbroker [received data] from MATLAB = %d\n", size);

//             // //deconcatenate samples from MATLAB

//             std::vector<std::vector<cf_t>> deconcatenate_samples = deconcatenate_all_samples(matlab_buffer, packet_sizes);

//             // //send data to ues

//             for(int i = 0; i < num_ues; ++i){
//                 send = zmq_send(send_sockets_for_ue_rx[i], deconcatenate_samples[0].data(), packet_sizes[0], 0);

//                 if(send == -1){
//                     printf("ERROR: Cannot transmit data to UE%d", i + 1);
//                     continue;
//                 }

//                 ues[i].set_samples_rx(deconcatenate_samples[0], send);
//                 printf("send_socket_for_ue_%d_rx [send data] = %d\n", i + 1 ,send);
//             }

//             // //concatenate ues sameples

//             int max_size = -1;

//             //find largest packet
//             for(int i = 1; i < packet_sizes.size(); ++i){
//                 if(packet_sizes[i] > max_size){
//                     max_size = packet_sizes[i];
//                 }
//             }

//             printf("max ues packet size: %d\n", max_size);

//             concatenate_buffer.clear();

//             concatenate_buffer.resize(max_size / sizeof(cf_t));

//             for(int i = num_gnbs; i < num_gnbs + num_ues; ++i){
//                 transform(concatenate_buffer.begin(), concatenate_buffer.end(), deconcatenate_samples[i].begin(), concatenate_buffer.begin(), std::plus<std::complex<float>>());
//             }

//             // //send data to gnb

//             send = zmq_send(send_socket_for_gnb_rx, (void*)concatenate_buffer.data(), max_size, 0);

//             gnbs[0].set_samples_rx(concatenate_buffer, send);

//             printf("\nsend_socket_for_gnb_rx [send data] = %d\n", send);
            
//         }
           
//         else
//         {   
//             continue;
//         }

//     }

// }
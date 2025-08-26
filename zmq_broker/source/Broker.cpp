#include "../includes/Broker.hpp"

int Broker::broker_count = 0;

Broker::Broker(std::vector<UserEquipment>& _ues, std::vector<gNodeB>& _gnbs, int _matlab_port){

    if(broker_count == 1){
        printf("Broker already exist");
    }
    
    else{
        ues = _ues;
        gnbs = _gnbs;
    
        broker_count++;

        matlab_port = _matlab_port;
    }

}

Broker::Broker(){};

//geters
std::vector<gNodeB> Broker::get_list_of_gnbs() const{
    return gnbs;
}

std::vector<UserEquipment> Broker::get_list_of_ues() const{
    return ues;
}

//setters
void Broker::set_list_of_gnbs(std::vector<gNodeB> _gnbs){
    gnbs = _gnbs;
}

void Broker::set_list_of_ues(std::vector<UserEquipment> _ues){
    ues = _ues;
}

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

int Broker::receive_from_matlab(void *socket_to_matlab, std::vector<uint8_t> &data, int target_rcv_data) {
    zmq_msg_t recv_id, recv_data;
    int sum_rcv_data = 0;
    int rc;

    data.clear();

    while (sum_rcv_data < target_rcv_data) {
        zmq_msg_init(&recv_id);
        rc = zmq_msg_recv(&recv_id, socket_to_matlab, 0);
        if (rc <= 0) {
            zmq_msg_close(&recv_id);
            fprintf(stderr, "\nFailed to receive ID\n");
            continue;
        }
        zmq_msg_close(&recv_id); 

        zmq_msg_init(&recv_data);
        rc = zmq_msg_recv(&recv_data, socket_to_matlab, 0);
        if (rc <= 0) {
            zmq_msg_close(&recv_data);
            fprintf(stderr, "\nFailed to receive packet\n");
            break;
        }

        size_t recv_size = zmq_msg_size(&recv_data);
        uint8_t* recv_ptr = static_cast<uint8_t*>(zmq_msg_data(&recv_data));

        int bytes_to_copy = std::min(static_cast<int>(recv_size), target_rcv_data - sum_rcv_data);

        data.insert(data.end(), recv_ptr, recv_ptr + bytes_to_copy);
        sum_rcv_data += bytes_to_copy;

        printf("socket_to_matlab [receive data] = %d (total = %d)\n", bytes_to_copy, sum_rcv_data);

        if (bytes_to_copy < static_cast<int>(recv_size)) {
            size_t extra = recv_size - bytes_to_copy;
            printf("\nExtra data (%zu bytes) ignored\n", extra);
            zmq_msg_close(&recv_data);
            break;
        }

        zmq_msg_close(&recv_data);
    }

    return sum_rcv_data;
}

template <typename T>
std::vector<uint8_t> Broker::to_byte(std::vector<T> data){

    int byte_size = data.size() * sizeof(T);

    std::vector<uint8_t> result(byte_size);

    memcpy(result.data(), data.data(), byte_size);

    return result;
}

std::vector<int> Broker::get_tx_samples_sizes() const{
    std::vector<int> result;

    for(const UserEquipment& el : ues){
        result.push_back(el.get_samples_tx().size());
    }

    return result;
}

std::vector<int> Broker::get_rx_samples_sizes() const{
    std::vector<int> result;

    for(const UserEquipment& el : ues){
        result.push_back(el.get_samples_rx().size());
    }

    return result;
}

//get tx samples from ues and gnbs
std::vector<std::vector<std::complex<float>>> Broker::get_all_tx_samples() const{

    std::vector<std::vector<std::complex<float>>> result;

    for(const UserEquipment& el: ues){
        result.push_back(el.get_samples_tx());
    }

    for(const gNodeB& el: gnbs){
        result.push_back(el.get_samples_tx());
    }

    return result;
}

std::vector<uint8_t> Broker::concatenate_tx_samples(){

    std::vector<std::vector<std::complex<float>>> all_samples = get_all_tx_samples();

    int len = 0; //len samples in bytes

    for(const std::vector<std::complex<float>>& el : all_samples){
        len += el.size() + 1;                                           // +1 for ID
    }

    std::vector<uint8_t> result(len);

    int offset = 0;

    for(int i = 0; i < all_samples.size(); ++i){

        result[offset] = static_cast<uint8_t>(i);

        offset++;

        memcpy(result.data() + offset, all_samples[i].data(), all_samples[i].size() * sizeof(std::complex<float>));
        offset += all_samples[i].size() * sizeof(std::complex<float>);
    }

    return result;
}


std::vector<std::vector<std::complex<float>>> Broker::deconcatenate_all_samples(std::vector<uint8_t> &all_samples, std::vector<int> packet_sizes) {

    std::vector<std::vector<std::complex<float>>> result(packet_sizes.size());
    int offset = 0;

    for(int i = 0; i < packet_sizes.size(); ++i){
        result[i].resize(packet_sizes[i]);
        memcpy(result[i].data(), all_samples.data() + offset + 1, packet_sizes[i]);

        offset += packet_sizes[i];
    }

    return result;

}

void Broker::run(){

    if(!(gnbs.size() > 0 && ues.size() > 0)){
        printf("Broker dont have gnbs or ues list");
        return;
    }

    int num_ues = ues.size();
    int num_gnbs = gnbs.size();

    bool matlab_enable = matlab_port == -1 ? false : true;

    std::string addr_recv_port_gnb_tx = "tcp://localhost:" + std::to_string(gnbs[0].get_tx_port());
    std::string addr_send_port_gnb_rx = "tcp://*:" + std::to_string(gnbs[0].get_rx_port());

    //Address
    std::vector<std::string> addr_recv_ports_ue_tx(num_ues);
    std::vector<std::string> addr_send_ports_ue_rx(num_ues);


    for(int i = 0; i < num_ues; i++){
        addr_send_ports_ue_rx[i] = "tcp://*:" + std::to_string(ues[i].get_rx_port());
        addr_recv_ports_ue_tx[i] = "tcp://localhost:" + std::to_string(ues[i].get_tx_port());
    }

    // Initialize ZMQ
    void *context = zmq_ctx_new ();

    void *req_socket_from_gnb_tx = zmq_socket(context, ZMQ_REQ);

    std::vector<void*> req_sockets_from_ue_tx(num_ues);

    for(int i = 0; i < num_ues; ++i){
        req_sockets_from_ue_tx[i] = zmq_socket(context, ZMQ_REQ);
    }

    if(zmq_connect(req_socket_from_gnb_tx, addr_recv_port_gnb_tx.c_str())){
        perror("zmq_connect\n");
        return;
    } else {
        printf("zmq_connect [req_socket_for_gnb_tx] - success\n");
    }

    for(int i = 0; i < num_ues; ++i){
        if(zmq_connect(req_sockets_from_ue_tx[i], addr_recv_ports_ue_tx[i].c_str())){
            perror("zmq_connect\n");
            return;
        } else {
            printf("zmq_connect [req_socket_for_UE%d_tx] - success\n", i+1);
        }   
    }

    void *send_socket_for_gnb_rx = zmq_socket(context, ZMQ_REP);

    std::vector<void*>send_sockets_for_ue_rx(num_ues);

    for(int i = 0; i < num_ues; ++i){
        send_sockets_for_ue_rx[i] = zmq_socket(context, ZMQ_REP);
    }

    if(zmq_bind(send_socket_for_gnb_rx, addr_send_port_gnb_rx.c_str())) {
        perror("zmq_bind\n");
        return;
    } else {
        printf("zmq_bind [send_socket_for_gnb_rx]- success\n");
    }

    for(int i = 0; i < num_ues; ++i){
        if(zmq_bind(send_sockets_for_ue_rx[i], addr_send_ports_ue_rx[i].c_str())){
            perror("zmq_connect\n");
            return;
        } else {
            printf("zmq_connect [req_socket_for_UE%d_rx] - success\n", i+1);
        }   
    }

    //socket to matlab

    void *socket_to_matlab;

    if(matlab_enable){

        socket_to_matlab = zmq_socket (context, ZMQ_STREAM);
    
        std::string matlab_address = "tcp://*:" + std::to_string(matlab_port);
    
        if(zmq_bind(socket_to_matlab, matlab_address.data())){
            perror("zmq_bind");
            return;
        } else {
            printf("zmq_bind [matlab_socket] - success\n");
        }
    }

    int timeout = 25000;

    zmq_setsockopt(req_socket_from_gnb_tx, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

    for(int i = 0; i < num_ues; ++i){
        zmq_setsockopt(req_sockets_from_ue_tx[i], ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    }

    using cf_t = std::complex<float>;

    int N = 80000;

    int nbytes = N * sizeof(cf_t);

    std::vector<cf_t> buffer_vec(N);
    std::vector<cf_t> concatenate_buffer(N);

    char buffer_acc[10];
    int broker_rcv_accept_ues[num_ues] = {0};
    int broker_rcv_accept_gnbs[num_gnbs] = {0};

    int size;

    std::vector<uint8_t> concatenate_samples;

    std::vector<uint8_t> byte_packet_size;

    bool matlab_connected = false;

    std::vector<uint8_t> matlab_buffer(buffer_vec.size() * sizeof(cf_t) + 1);

    std::vector<int> packet_sizes;

    zmq_msg_t id_msg;

    size_t id_size;
    char *id_data;


    while(1){

        //check matlab conection if he is enable

        if(!matlab_connected && matlab_enable){

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

        //receive conn accept from RX's

        memset(buffer_acc, 0, sizeof(buffer_acc));

        size = zmq_recv(send_socket_for_gnb_rx, buffer_acc, sizeof(buffer_acc), 0);

        if(size == -1){

            printf("!!!!!!!!!!!   -----1 send_socket_for_gnb_rx\n");

            printf(zmq_strerror(zmq_errno()));

            continue;

        } else{

            broker_rcv_accept_gnbs[0] = 1;

            printf("broker received [buffer_acc] form GNB RX = %d\n", size);
        }

        for(int i = 0; i < num_ues; ++i){

            memset(buffer_acc, 0, sizeof(buffer_acc));

            size = zmq_recv(send_sockets_for_ue_rx[i], buffer_acc, sizeof(buffer_acc), 0);

            if(size == -1){

                printf("!!!!!!!!!!!   -----1 send_socket_for_ue_%d_rx\n", i+1);

                continue;

            } else{
                broker_rcv_accept_ues[i] = 1;

                printf("broker received [buffer_acc] from UE[%d] RX = %d\n", i+1, size);
            }
        }
    

        int summ = 0;

        for (int i = 0; i < num_ues; i++){
            summ += broker_rcv_accept_ues[i];
        }

        printf("Accept UES: %d\n", summ);

        if (summ == num_ues && broker_rcv_accept_gnbs[0] == num_gnbs)
        {
            int send = 0;

            // send accepts to TX's
            send = zmq_send(req_socket_from_gnb_tx, buffer_acc, size, 0);

            printf("req_socket_from_gnb_tx [send] = %d\n", send);

            for(int i = 0; i < num_ues; ++i){
                send = zmq_send(req_sockets_from_ue_tx[i], buffer_acc, size, 0);
                printf("req_socket_from_ue_%d_tx [send] = %d\n", i+1 ,send);
            }


            // start data transmissiona

            fill(buffer_vec.begin(), buffer_vec.end(), 0);

            //recv from gnb
            size = zmq_recv(req_socket_from_gnb_tx, (void*)buffer_vec.data(), nbytes, 0);

            if (size == -1){
                printf("ERROR: broker received from gNb = %d\n bytes", size);
                continue;
            }

            gnbs[0].set_samples_tx(buffer_vec, size);

            printf("broker received from gNb %d\n bytes", size);

            printf("\nsamples from gnb\n");
            
            //check first 100 samples
            for(int j = 0; j < 100; ++j){
                std::cout << gnbs[0].get_samples_tx()[j] << ", ";
            }

            printf("\n");

            //recieve samples from ues
            for(int i = 0; i < num_ues; ++i){

                fill(buffer_vec.begin(), buffer_vec.end(), 0);

                size = zmq_recv(req_sockets_from_ue_tx[i], (void*)buffer_vec.data(), nbytes, 0);

                if (size == -1){
                    printf("ERROR: Broker recieved from UE%d %d bytes", i + 1, size);
                    continue;
                }

                ues[i].set_samples_tx(buffer_vec, size);

                printf("\nbroker [received data] from UE[%d] %d size packet\n", i+1 ,size);
            }

            //send to MATLAB packet sizes

            packet_sizes = get_tx_samples_sizes();

            byte_packet_size = to_byte(packet_sizes);

            size = send_to_matlab(socket_to_matlab, byte_packet_size, byte_packet_size.size(), id_data, id_size);

            printf("\nsocket to matlab [send data] (packet size) = %d\n", size);

            //concatenate ues and gnb samples

            concatenate_samples = concatenate_tx_samples();

            printf("concatenate samples size = %d", concatenate_samples.size());

            // //send all samples to MATLAB

            size = send_to_matlab(socket_to_matlab, concatenate_samples, concatenate_samples.size(), id_data, id_size);

            printf("\nsocket to matlab [send data] = %d\n", size);

            // //receive all samples from matlab

            matlab_buffer.resize(size);

            size = receive_from_matlab(socket_to_matlab, matlab_buffer, size);

            printf("\nbroker [received data] from MATLAB = %d\n", size);

            // //deconcatenate samples from MATLAB

            std::vector<std::vector<cf_t>> deconcatenate_samples = deconcatenate_all_samples(matlab_buffer, packet_sizes);

            // //send data to ues

            for(int i = 0; i < num_ues; ++i){
                send = zmq_send(send_sockets_for_ue_rx[i], deconcatenate_samples[0].data(), packet_sizes[0], 0);

                if(send == -1){
                    printf("ERROR: Cannot transmit data to UE%d", i + 1);
                    continue;
                }

                ues[i].set_samples_tx(deconcatenate_samples[0], send);
                printf("send_socket_for_ue_%d_rx [send data] = %d\n", i + 1 ,send);
            }

            // //concatenate ues sameples

            int max_size = -1;

            //find largest packet
            for(int i = 1; i < packet_sizes.size(); ++i){
                if(packet_sizes[i] > max_size){
                    max_size = packet_sizes[i];
                }
            }

            printf("max ues packet size: %d\n", max_size);

            concatenate_buffer.clear();

            concatenate_buffer.resize(max_size / sizeof(cf_t));

            for(int i = num_gnbs; i < num_gnbs + num_ues; ++i){
                transform(concatenate_buffer.begin(), concatenate_buffer.end(), deconcatenate_samples[i].begin(), concatenate_buffer.begin(), std::plus<std::complex<float>>());
            }

            // //send data to gnb

            send = zmq_send(send_socket_for_gnb_rx, (void*)concatenate_buffer.data(), max_size, 0);

            gnbs[0].set_samples_rx(concatenate_buffer, send);

            printf("\nsend_socket_for_gnb_rx [send data] = %d\n", send);
            
        }
           
        else
        {   
            continue;
        }

        usleep(100);
    }

}
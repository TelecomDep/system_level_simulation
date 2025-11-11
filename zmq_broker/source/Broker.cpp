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
    //std::fill(concatenate_to_gnb_samples.begin(), concatenate_to_gnb_samples.end(), 0);
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

            //std::fill(buffer_acc.begin(), buffer_acc.end(), 0);
            int size = zmq_recv(matlab_req_socket, (void *)buffer_acc.data(), N, 0);
            if(size == -1){
                printf("Error recv from matlab\n");
            } else {
                printf("revc from matlab accept = %d\n", size);
            }


            if(max_size < ues[i].get_recv_nbytes()){
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
    std::fill(concatenate_to_gnb_samples.begin(), concatenate_to_gnb_samples.end(), 0);
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

    if(enable_matlab){
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
    }
    

    gnbs[0].initialize_sockets(zmq_context);
    gnbs[0].activate();
    for (int i = 0; i < ues.size(); i++)
    {
        ues[i].initialize_sockets(zmq_context);
        ues[i].activate();
    }
}

void Broker::async_recv_conn_request_from_reqs()
{
    gnbs[0].rep_recv_conn_request_from_req();

    for(int i = 0; i < ues.size();i++)
    {
        ues[i].rep_recv_conn_request_from_req();
    }
}

void Broker::async_send_request_for_samples_ang_get_samples()
{
    uint8_t dummy = 255;
    int dummy_size = 1;
    bool check = false;

    for (int i = 0; i < ues.size(); i++)
    {
        //if(ues[i].is_rx_ready()){
            check = true;
            dummy = ues[i].dummy;
            dummy_size = ues[i].dummy_size;
            //break;
        //}
    }
    gnbs[0].send_req_to_get_samples_from_rep(dummy, dummy_size);
    
    for(int i = 0; i < ues.size();i++)
    {
        //if(gnbs[0].is_rx_ready()){
            dummy = gnbs[0].dummy;
            dummy_size = gnbs[0].dummy_size;
            ues[i].send_req_to_get_samples_from_rep(dummy, dummy_size);
        //}
    }
}

void Broker::recv_conn_request_from_gnb()
{
    gnbs[0].rep_recv_conn_request_from_req();
}

void Broker::recv_conn_request_from_ues()
{
    for(int i = 0; i < ues.size();i++)
    {
        ues[i].rep_recv_conn_request_from_req();
    }
}

void Broker::send_request_for_samples_and_get_samples_from_gnb()
{
    uint8_t dummy = 0;
    int dummy_size = 0;
    bool check = false;
    for (int i = 0; i < ues.size(); i++)
    {
        if(ues[i].is_rx_ready()){
            check = true;
            dummy = ues[i].dummy;
            dummy_size = ues[i].dummy_size;
            break;
        }
    }
    if(check){
        gnbs[0].send_req_to_get_samples_from_rep(dummy, dummy_size);
    }
    
}

void Broker::send_request_for_samples_and_get_samples_from_ues()
{
    uint8_t dummy = 0;
    int dummy_size = 0;
    for(int i = 0; i < ues.size();i++)
    {
        if(gnbs[0].is_rx_ready()){
            dummy = gnbs[0].dummy;
            dummy_size = gnbs[0].dummy_size;
            ues[i].send_req_to_get_samples_from_rep(dummy, dummy_size);
        }
    }
}


void Broker::async_send_samples_to_all_ues()
{
    bool gnb_samples_ready = gnbs[0].is_tx_samples_ready();
    //if(gnbs[0].is_tx_samples_ready()) {
        for (int i = 0; i < ues.size(); i++)
        {
            if(gnbs[0].is_tx_samples_ready()) { 
                //std::fill(matlab_samples.begin(), matlab_samples.end(), 0);
                matlab_samples = gnbs[0].samples_to_transmit;
                float pl = (i+1) * 10.0f;
                for (int i = 0; i < matlab_samples.size(); i++){
                    std::complex<float> val_1;
                    val_1 = std::complex<float>(matlab_samples[i].real()/pl, matlab_samples[i].imag()/pl);
                    matlab_samples[i] = val_1;
                }
                ues[i].send_samples_to_req_rx(matlab_samples, gnbs[0].get_ready_to_tx_bytes());
                //ues[i].send_samples_to_req_rx(gnbs[0].get_samples_tx(), gnbs[0].get_ready_to_tx_bytes());
            }   
        }
    //}
}

void Broker::async_send_concatenated_sampled_from_ues_to_gnb()
{
    int max_size = 0;
    
    // Ищем самый большой размер пакета, чтобы не потерять часть сэмплов при суммировании
    std::fill(concatenate_to_gnb_samples.begin(), concatenate_to_gnb_samples.end(), 0);
    // Суммируем
    bool check = false;
    for (int i = 0; i < ues.size(); i++)
    {
        if(ues[i].is_tx_samples_ready() && all_ues_samples_received){
        
            check = true;
            if (max_size < ues[i].get_ready_to_tx_bytes())
            {
                max_size = ues[i].get_ready_to_tx_bytes();
            }

            ues[i].divide_samples_by_value((i+1)*10.0f); // path losses

            std::transform( concatenate_to_gnb_samples.begin(), 
                            concatenate_to_gnb_samples.end(), 
                            ues[i].get_samples_tx().begin(), 
                            concatenate_to_gnb_samples.begin(), 
                            std::plus<std::complex<float>>());

        }
    }

    // Отправляем сумму сэмплов на базовую станцию
    if(check){
        gnbs[0].send_samples_to_req_rx(concatenate_to_gnb_samples, max_size);
        all_ues_samples_ready = 0;
        all_ues_samples_received = false;
        // gnbs[0].send_samples_to_req_rx(ues[0].get_samples_tx(), ues[0].get_ready_to_tx_bytes());
    }
}

void Broker::async_check_ues_samples_ready()
{
    for (int i = 0; i < ues.size(); i++)
    {
        if(ues[i].is_tx_samples_ready()){
            all_ues_samples_ready++;
        }
    }
    if(all_ues_samples_ready == ues.size()){
        all_ues_samples_received = true;
    }
}   

void Broker::run_async_world()
{
    broker_working_counter = 0;
    while (is_running)
    {
        if(enable_matlab){

        } else 
        {
            std::cout << "------------------Начало---------------------" << std::endl;
            std::cout << "------------->>получили запросы от RX-ов" << std::endl;
            async_recv_conn_request_from_reqs();
            std::cout << "------------->>отправили запросы и получили сэмплы" << std::endl;
            async_send_request_for_samples_ang_get_samples();

            std::cout << "------------->>отправили сэмплы от gNb до UEs" << std::endl;
            async_send_samples_to_all_ues();
            std::cout << "------------->>отправили сэмплы от UEs до gNb" << std::endl;
            async_check_ues_samples_ready();
            async_send_concatenated_sampled_from_ues_to_gnb();
            std::cout << "------------------Конец---------------------" << std::endl;
            
            
        }
    }
}

// void Broker::run_the_world()
// {
//     broker_working_counter = 0;
//     while (is_running)
//     {
//         if(enable_matlab){
//             std::cout << "---------------------------------------" << std::endl;
//             if (recv_conn_accepts())
//             {
//                 std::cout << "------------->>send_conn_accepts()" << std::endl;
//                 send_conn_accepts();

//                 std::cout << "------------->>recv_samples_from_gNb()" << std::endl;
//                 recv_samples_from_gNb();
//                 send_from_gnb_to_matlab_per_ue();

//                 std::cout << "------------->>recv_samples_from_ues()" << std::endl;
//                 recv_samples_from_ues();

//                 send_from_ues_to_matalb_and_send_to_gnb();
//             } else {
//                 continue;
//             }
//         }
//         broker_working_counter++;
//     }
// }

void Broker::start_the_proxy()
{
    initialize_zmq_sockets();
    run_async_world();
    //run_the_world();
}

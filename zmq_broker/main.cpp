#include "ZmqRadioBroker.hpp"
#include "Config.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>

static volatile sig_atomic_t g_running = 1;
void sig_handler(int) { g_running = 0; }

int main(int argc, char* argv[]) {
    signal(SIGINT, sig_handler);

    std::string config_path = (argc > 1) ? argv[1] : "../config.yaml";
    
    BrokerConfig config;
    try {
        config = BrokerConfig::load(config_path);
        std::cout << "[Config] Loaded: " << config_path << "\n";
    } catch (const std::exception& e) {
        std::cerr << "[Config Error] " << e.what() << "\n";
        return 1;
    }

    void* zmq_ctx = zmq_ctx_new();

    std::string gnb_tx = "tcp://127.0.0.1:" + std::to_string(config.gnb_tx_port);
    std::string gnb_rx = "tcp://*:" + std::to_string(config.gnb_rx_port);

    ZmqRadioBroker broker(zmq_ctx, gnb_tx, gnb_rx);

    if (config.matlab_enabled) {
        broker.enable_matlab_channel(zmq_ctx, config.matlab_endpoint);
    }

    for (const auto& ue : config.ues) {
        std::string tx_addr = "tcp://127.0.0.1:" + std::to_string(ue.tx_port);
        std::string rx_addr = "tcp://*:" + std::to_string(ue.rx_port);

        broker.add_ue(zmq_ctx, ue.id, tx_addr, rx_addr, static_cast<float>(ue.distance_km), ue.path_loss_lin);
        std::cout << "[Broker] Registered UE[" << ue.id << "] (d=" << ue.distance_km 
                  << " km, linear PL=" << ue.path_loss_lin << ")\n";
    }

    std::cout << "[Broker] Running. Press Ctrl+C to stop.\n";

    while (g_running) {
        broker.process_cycle();
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    zmq_ctx_destroy(zmq_ctx);
    std::cout << "\n[Broker] Terminated cleanly.\n";
    return 0;
}

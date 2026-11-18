#pragma once

#include "RadioEndpoint.hpp"
#include <vector>
#include <string>

class ZmqRadioBroker {
public:
    ZmqRadioBroker(void* zmq_ctx, const std::string& gnb_tx_addr, const std::string& gnb_rx_addr);
    ~ZmqRadioBroker();

    void enable_matlab_channel(void* zmq_ctx, const std::string& matlab_addr);
    void add_ue(void* zmq_ctx, int id, const std::string& tx_addr, const std::string& rx_addr, float dist_km, float pl_lin);
    
    void process_cycle();
    bool accumulate_uplink();
    void process_downlink();

private:
    bool filter_channel_matlab(int ue_id, int direction, float distance_km, std::vector<cf_t>& samples);

    RadioEndpoint gnb_;
    std::vector<RadioEndpoint> ues_;

    void* matlab_sock_ = nullptr;
    bool use_matlab_ = false;

    std::vector<cf_t> tmp_pull_buf_;
    std::vector<cf_t> tmp_slot_buf_;
    std::vector<cf_t> ue_slot_buf_;
    std::vector<cf_t> combined_ul_slot_;
};

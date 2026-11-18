#include "ZmqRadioBroker.hpp"
#include "ChannelProtocol.hpp"
#include <iostream>

ZmqRadioBroker::ZmqRadioBroker(void* zmq_ctx, const std::string& gnb_tx_addr, const std::string& gnb_rx_addr)
    : gnb_(zmq_ctx, -1, gnb_tx_addr, gnb_rx_addr, 0.0f, 1.0f) {
    tmp_pull_buf_.resize(RadioEndpoint::MAX_PULL_SAMPLES);
    tmp_slot_buf_.resize(RadioEndpoint::SAMPLES_PER_SLOT);
    ue_slot_buf_.resize(RadioEndpoint::SAMPLES_PER_SLOT);
}

ZmqRadioBroker::~ZmqRadioBroker() {
    if (matlab_sock_) {
        zmq_close(matlab_sock_);
    }
}

void ZmqRadioBroker::enable_matlab_channel(void* zmq_ctx, const std::string& matlab_addr) {
    matlab_sock_ = zmq_socket(zmq_ctx, ZMQ_REQ);
    int timeout = 500;
    zmq_setsockopt(matlab_sock_, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    zmq_setsockopt(matlab_sock_, ZMQ_SNDTIMEO, &timeout, sizeof(timeout));

    if (zmq_connect(matlab_sock_, matlab_addr.c_str()) == 0) {
        use_matlab_ = true;
        std::cout << "[Broker] Connected to MATLAB Channel Engine at " << matlab_addr << "\n";
    }
}

void ZmqRadioBroker::add_ue(void* zmq_ctx, int id, const std::string& tx_addr, const std::string& rx_addr, float dist_km, float pl_lin) {
    ues_.emplace_back(zmq_ctx, id, tx_addr, rx_addr, dist_km, pl_lin);
}

bool ZmqRadioBroker::filter_channel_matlab(int ue_id, int direction, float distance_km, std::vector<cf_t>& samples) {
    if (!use_matlab_ || !matlab_sock_) return false;

    ChannelRequestHeader hdr{
        ue_id,
        direction,
        distance_km,
        static_cast<int32_t>(samples.size())
    };

    int s1 = zmq_send(matlab_sock_, &hdr, sizeof(hdr), ZMQ_SNDMORE);
    int s2 = zmq_send(matlab_sock_, samples.data(), samples.size() * sizeof(cf_t), 0);

    if (s1 < 0 || s2 < 0) return false;

    int bytes = zmq_recv(matlab_sock_, samples.data(), samples.size() * sizeof(cf_t), 0);
    return (bytes == static_cast<int>(samples.size() * sizeof(cf_t)));
}

void ZmqRadioBroker::process_cycle() {
    gnb_.poll_tx(tmp_pull_buf_);
    for (auto& ue : ues_) {
        ue.poll_tx(tmp_pull_buf_);
    }

    gnb_.poll_rx_request();
    for (auto& ue : ues_) {
        ue.poll_rx_request();
    }

    process_downlink();
    accumulate_uplink();
}

bool ZmqRadioBroker::accumulate_uplink() {
    if (!gnb_.is_rx_pending()) return false;

    const size_t slot_size = gnb_.requested_samples();

    for (const auto& ue : ues_) {
        if (!ue.has_tx_samples(slot_size)) {
            return false;
        }
    }

    combined_ul_slot_.assign(slot_size, cf_t(0.0f, 0.0f));

    for (auto& ue : ues_) {
        ue.pop_raw_tx_samples(ue_slot_buf_, slot_size);

        if (use_matlab_) {
            filter_channel_matlab(ue.getId(), 1 /*UL*/, ue.getDistance(), ue_slot_buf_);
        } else {
            ue.apply_local_pathloss(ue_slot_buf_);
        }

        volk_32fc_x2_add_32fc(
            reinterpret_cast<lv_32fc_t*>(combined_ul_slot_.data()),
            reinterpret_cast<const lv_32fc_t*>(combined_ul_slot_.data()),
            reinterpret_cast<const lv_32fc_t*>(ue_slot_buf_.data()),
            slot_size
        );
    }

    gnb_.push_rx_samples(combined_ul_slot_.data(), slot_size);
    return gnb_.flush_rx_response(tmp_slot_buf_);
}

void ZmqRadioBroker::process_downlink() {
    if (gnb_.has_tx_samples(RadioEndpoint::SAMPLES_PER_SLOT)) {
        gnb_.pop_raw_tx_samples(tmp_slot_buf_, RadioEndpoint::SAMPLES_PER_SLOT);

        for (auto& ue : ues_) {
            std::vector<cf_t> dl_slot = tmp_slot_buf_;
            if (use_matlab_) {
                filter_channel_matlab(ue.getId(), 0 /*DL*/, ue.getDistance(), dl_slot);
            } else {
                ue.apply_local_pathloss(dl_slot);
            }
            ue.push_rx_samples(dl_slot.data(), RadioEndpoint::SAMPLES_PER_SLOT);
        }
    }

    for (auto& ue : ues_) {
        ue.flush_rx_response(tmp_slot_buf_);
    }
}

#pragma once

#include <complex>
#include <vector>
#include <deque>
#include <string>
#include <zmq.h>
#include <volk/volk.h>

using cf_t = std::complex<float>;

class RadioEndpoint {
public:
    static constexpr int SAMPLES_PER_SLOT = 11520;
    static constexpr int RING_CAPACITY    = SAMPLES_PER_SLOT * 400;
    static constexpr int MAX_PULL_SAMPLES = SAMPLES_PER_SLOT * 8;
    static constexpr int ZMQ_TIMEOUT_MS   = 0;
    static constexpr int ZMQ_HWM          = 2000;

    RadioEndpoint(void* zmq_ctx, int id, const std::string& tx_pull_addr, const std::string& rx_rep_addr, float distance_km, float path_loss_lin);
    ~RadioEndpoint();

    RadioEndpoint(const RadioEndpoint&) = delete;
    RadioEndpoint& operator=(const RadioEndpoint&) = delete;

    RadioEndpoint(RadioEndpoint&& other) noexcept;
    RadioEndpoint& operator=(RadioEndpoint&& other) noexcept;

    int getId() const { return id_; }
    float getDistance() const { return distance_km_; }
    float getPathLoss() const { return pl_coeff_; }

    int poll_tx(std::vector<cf_t>& tmp_buf);
    bool has_tx_samples(size_t n) const { return tx_ring_.size() >= n; }
    
    // Extract raw samples without scaling (for MATLAB processing)
    void pop_raw_tx_samples(std::vector<cf_t>& out, size_t n);

    // Apply local linear path loss fallback
    void apply_local_pathloss(std::vector<cf_t>& buf);

    void push_rx_samples(const cf_t* samples, size_t n);
    void poll_rx_request();
    bool is_rx_pending() const { return rep_pending_; }
    int requested_samples() const { return requested_samples_; }
    bool flush_rx_response(std::vector<cf_t>& tmp_buf);

private:
    static void apply_sockopts(void* sock);

    int id_ = 0;
    void* sock_pull_ = nullptr;
    void* sock_rep_  = nullptr;
    float distance_km_ = 0.1f;
    float pl_coeff_  = 1.0f;

    bool req_waiting_ = false;
    bool rep_pending_ = false;
    int requested_samples_ = SAMPLES_PER_SLOT;

    std::deque<cf_t> tx_ring_;
    std::deque<cf_t> rx_ring_;
};

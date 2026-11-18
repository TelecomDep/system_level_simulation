#include "RadioEndpoint.hpp"
#include <stdexcept>
#include <utility>

RadioEndpoint::RadioEndpoint(void* zmq_ctx, int id, const std::string& tx_pull_addr, const std::string& rx_rep_addr, float distance_km, float path_loss_lin)
    : id_(id), distance_km_(distance_km), pl_coeff_(path_loss_lin) {
    sock_pull_ = zmq_socket(zmq_ctx, ZMQ_REQ);
    sock_rep_  = zmq_socket(zmq_ctx, ZMQ_REP);

    if (!sock_pull_ || !sock_rep_) {
        throw std::runtime_error("Failed to create ZMQ sockets for endpoint");
    }

    apply_sockopts(sock_pull_);
    apply_sockopts(sock_rep_);

    if (zmq_connect(sock_pull_, tx_pull_addr.c_str()) != 0) {
        throw std::runtime_error("Failed to connect REQ socket: " + tx_pull_addr);
    }
    if (zmq_bind(sock_rep_, rx_rep_addr.c_str()) != 0) {
        throw std::runtime_error("Failed to bind REP socket: " + rx_rep_addr);
    }
}

RadioEndpoint::~RadioEndpoint() {
    if (sock_pull_) zmq_close(sock_pull_);
    if (sock_rep_)  zmq_close(sock_rep_);
}

RadioEndpoint::RadioEndpoint(RadioEndpoint&& other) noexcept
    : id_(other.id_), sock_pull_(other.sock_pull_), sock_rep_(other.sock_rep_),
      distance_km_(other.distance_km_), pl_coeff_(other.pl_coeff_),
      req_waiting_(other.req_waiting_), rep_pending_(other.rep_pending_),
      requested_samples_(other.requested_samples_),
      tx_ring_(std::move(other.tx_ring_)), rx_ring_(std::move(other.rx_ring_)) {
    other.sock_pull_ = nullptr;
    other.sock_rep_  = nullptr;
}

RadioEndpoint& RadioEndpoint::operator=(RadioEndpoint&& other) noexcept {
    if (this != &other) {
        if (sock_pull_) zmq_close(sock_pull_);
        if (sock_rep_)  zmq_close(sock_rep_);

        id_ = other.id_;
        sock_pull_ = other.sock_pull_;
        sock_rep_  = other.sock_rep_;
        distance_km_ = other.distance_km_;
        pl_coeff_  = other.pl_coeff_;
        req_waiting_ = other.req_waiting_;
        rep_pending_ = other.rep_pending_;
        requested_samples_ = other.requested_samples_;
        tx_ring_ = std::move(other.tx_ring_);
        rx_ring_ = std::move(other.rx_ring_);

        other.sock_pull_ = nullptr;
        other.sock_rep_  = nullptr;
    }
    return *this;
}

void RadioEndpoint::apply_sockopts(void* sock) {
    int timeout = ZMQ_TIMEOUT_MS;
    int hwm = ZMQ_HWM;
    zmq_setsockopt(sock, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    zmq_setsockopt(sock, ZMQ_SNDTIMEO, &timeout, sizeof(timeout));
    zmq_setsockopt(sock, ZMQ_RCVHWM, &hwm, sizeof(hwm));
    zmq_setsockopt(sock, ZMQ_SNDHWM, &hwm, sizeof(hwm));
}

int RadioEndpoint::poll_tx(std::vector<cf_t>& tmp_buf) {
    if (!req_waiting_) {
        int ask = MAX_PULL_SAMPLES;
        int sent = zmq_send(sock_pull_, &ask, sizeof(ask), ZMQ_DONTWAIT);
        if (sent == sizeof(ask)) {
            req_waiting_ = true;
        } else {
            return 0;
        }
    }

    int bytes = zmq_recv(sock_pull_, tmp_buf.data(), tmp_buf.size() * sizeof(cf_t), ZMQ_DONTWAIT);
    if (bytes <= 0) return 0;

    req_waiting_ = false;
    int num_samples = bytes / sizeof(cf_t);

    for (int i = 0; i < num_samples; ++i) {
        if (tx_ring_.size() >= RING_CAPACITY) tx_ring_.pop_front();
        tx_ring_.push_back(tmp_buf[i]);
    }
    return num_samples;
}

void RadioEndpoint::pop_raw_tx_samples(std::vector<cf_t>& out, size_t n) {
    out.resize(n);
    for (size_t i = 0; i < n; ++i) {
        out[i] = tx_ring_.front();
        tx_ring_.pop_front();
    }
}

void RadioEndpoint::apply_local_pathloss(std::vector<cf_t>& buf) {
    volk_32fc_s32fc_multiply_32fc(
        reinterpret_cast<lv_32fc_t*>(buf.data()),
        reinterpret_cast<const lv_32fc_t*>(buf.data()),
        lv_32fc_t(pl_coeff_, 0.0f),
        buf.size()
    );
}

void RadioEndpoint::push_rx_samples(const cf_t* samples, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (rx_ring_.size() >= RING_CAPACITY) rx_ring_.pop_front();
        rx_ring_.push_back(samples[i]);
    }
}

void RadioEndpoint::poll_rx_request() {
    if (rep_pending_) return;

    int ask = 0;
    int bytes = zmq_recv(sock_rep_, &ask, sizeof(ask), ZMQ_DONTWAIT);
    if (bytes >= 0) {
        requested_samples_ = SAMPLES_PER_SLOT;
        rep_pending_ = true;
    }
}

bool RadioEndpoint::flush_rx_response(std::vector<cf_t>& tmp_buf) {
    if (!rep_pending_) return false;
    if (rx_ring_.size() < static_cast<size_t>(requested_samples_)) return false;

    tmp_buf.resize(requested_samples_);
    for (int i = 0; i < requested_samples_; ++i) {
        tmp_buf[i] = rx_ring_.front();
        rx_ring_.pop_front();
    }

    int sent = zmq_send(sock_rep_, tmp_buf.data(), tmp_buf.size() * sizeof(cf_t), ZMQ_DONTWAIT);
    if (sent >= 0) {
        rep_pending_ = false;
        return true;
    }
    return false;
}

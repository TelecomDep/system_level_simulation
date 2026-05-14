#include <atomic>
#include <cmath>
#include <complex>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <string>
#include <thread>
#include <vector>
#include <random>
#include <zmq.h>
#include <volk/volk.h>

using cf_t = std::complex<float>;

const int SAMPLE_RATE = 11520000;
const int SAMPLES_PER_SLOT = SAMPLE_RATE / 1000;
const int RING_CAPACITY = SAMPLES_PER_SLOT * 400;
const int MAX_PULL_SAMPLES = SAMPLES_PER_SLOT * 8;
const int ZMQ_TIMEOUT_MS = 0;
const int ZMQ_HWM = 2000;

void my_handler(int s){
    printf("Caught signal %d\n",s);
    
    exit(1); 

}

struct Req {
  void* sock = nullptr;
  bool waiting = false; // ready or wait
  int rx_samples = 0;
};

struct Rep {
  void* sock = nullptr;
  bool request_pending = false;
  int requested_samples = 0;
  int tx_samples = 0;
};

struct UePath {
  double d; 
  double pl = 1.0f;
  Req ul_req;
  Rep dl_rep;
  std::deque<cf_t> ul_ring;
  std::deque<cf_t> dl_ring;
  std::vector<cf_t> slot_buf;
};

double db_to_pl(double db) { 
  return std::pow(10.0f, -db / 20.0f); 
}

int req_try_pull(Req& req, std::vector<cf_t>& rx_tmp){
  if (!req.waiting) {
    int ask = MAX_PULL_SAMPLES;
    int sent = zmq_send(req.sock, &ask, sizeof(ask), ZMQ_DONTWAIT);
    if (sent == sizeof(ask)) {
      req.waiting = true;
    } else if (sent < 0) {
      return 0;
    }
  }

  int recv_bytes = zmq_recv(req.sock, rx_tmp.data(), rx_tmp.size() * sizeof(cf_t), ZMQ_DONTWAIT);
  if (recv_bytes < 0) {
    return 0;
  }

  req.waiting = false;
  if (recv_bytes % sizeof(cf_t) != 0) {
    return 0;
  }
  int ns = recv_bytes / sizeof(cf_t);
  req.rx_samples = ns;
  return ns;
}

void multiply_by_const(std::vector<cf_t>& input, float coeff) {
    if (input.empty()) return;

    volk_32fc_s32fc_multiply_32fc(
        reinterpret_cast<lv_32fc_t*>(input.data()),
        reinterpret_cast<const lv_32fc_t*>(input.data()),
        lv_32fc_t(coeff, 0.0f),
        input.size()
    );
}

void add_vectors_volk(std::vector<cf_t>& vec1, const std::vector<cf_t>& vec2) {
    unsigned int size = vec1.size();
    if (size == 0 || size > vec1.size() || size > vec2.size()) return;

    volk_32fc_x2_add_32fc(
        reinterpret_cast<lv_32fc_t*>(vec1.data()),
        reinterpret_cast<const lv_32fc_t*>(vec1.data()),
        reinterpret_cast<const lv_32fc_t*>(vec2.data()),
        size
    );
}

void ring_push(std::deque<cf_t>& ring, cf_t* in, int n){
  for (int i = 0; i < n; ++i) {
    if (ring.size() >= RING_CAPACITY) {
      ring.pop_front();
    }
    ring.push_back(in[i]);
  }
}

bool ring_pop_exact(std::deque<cf_t>& ring, std::vector<cf_t>& out, int n){
  if (ring.size() < n) {
    return 0;
  }
  out.resize(n);
  for (int i = 0; i < n; ++i) {
    out[i] = ring.front();
    ring.pop_front();
  }
  return 1;
}

int rep_try_recv_request(Rep& rep){
  if (rep.request_pending) {
    return 0;
  }
  int ask = 0;
  int recv_bytes = zmq_recv(rep.sock, &ask, sizeof(ask), ZMQ_DONTWAIT);
  if (recv_bytes < 0) {
    return 0;
  }

  rep.requested_samples = SAMPLES_PER_SLOT;
  rep.request_pending = true;
  return 1;
}

int rep_send_response(Rep& rep, cf_t* data, int n_samples){
  if (!rep.request_pending) {
    return 0;
  }
  int to_send = std::min(rep.requested_samples, n_samples);
  int sent = zmq_send(rep.sock, data, to_send * sizeof(cf_t), ZMQ_DONTWAIT);
  if (sent < 0) {
    printf("send error");
    return 0;
  }
  rep.request_pending = false;
  rep.tx_samples = to_send;
  return 1;
}

bool set_sockopts(void* sock){
  int timeout = ZMQ_TIMEOUT_MS;
  int hwm = ZMQ_HWM;
  return zmq_setsockopt(sock, ZMQ_RCVTIMEO, &timeout, sizeof(timeout)) == 0 &&
         zmq_setsockopt(sock, ZMQ_SNDTIMEO, &timeout, sizeof(timeout)) == 0 &&
         zmq_setsockopt(sock, ZMQ_RCVHWM, &hwm, sizeof(hwm)) == 0 &&
         zmq_setsockopt(sock, ZMQ_SNDHWM, &hwm, sizeof(hwm)) == 0;
}


std::vector<cf_t> WGN(int num_samples, int N_0){
    double noise_power = pow(10.0, N_0 / 10.0);
    double std_dev_iq = std::sqrt(noise_power / 2.0); 

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::normal_distribution<double> dist(0.0, std_dev_iq);

    std::vector<cf_t> noise_vec(num_samples);

    for (int i = 0; i < num_samples; ++i){
        noise_vec[i] = cf_t(dist(gen), dist(gen));
    }

    return noise_vec;
}


double PL_coeff(double d){
    double A = 69.55;
    double B = 26.16;

    double f = 900;

    double hBs = 45;
    double hMs = 1.5;

    // double d = 0.01;

    double a = 1.1 * log10(f) * hMs - (1.56 * log10(f) - 0.8);
    double Lclutter = -(pow(4.78 * log10(11.75 * hMs), 2) - 18.33 * log10(f) + 40.94);
    double s = 44.9 - 6.55 * log10(hBs);

    double pl = A + B * log10(f) - 13.82 * log10(hBs) - a + s * log10(d) + Lclutter;

    return pl;
}

void parallel_pull(std::vector<UePath>& ues, size_t req_ns, std::vector<cf_t> noise){

  size_t num_threads = std::thread::hardware_concurrency();
  num_threads = std::min(num_threads, ues.size());

  std::vector<std::thread> threads;
  size_t chunk_size = (ues.size() + num_threads - 1) / num_threads;

  for(size_t t = 0; t < num_threads; t++){
    size_t start = t * chunk_size;
    size_t end = std::min(start + chunk_size, ues.size());

    threads.emplace_back([&](size_t s, size_t e) {
      for(size_t i = s; i < e; i++){
        auto& ue = ues[i];
        ue.slot_buf.resize(req_ns);
        ring_pop_exact(ue.ul_ring, ue.slot_buf, req_ns);
        multiply_by_const(ue.slot_buf, ue.pl);
        // add_vectors_volk(ue.slot_buf, noise);
      }
    }, start, end);
  }

  for (auto& th : threads) th.join();
}

void parallel_push(std::vector<UePath>& ues, std::vector<cf_t>* ptr_rx, 
                  std::vector<cf_t> noise, int gnb_ns){

  size_t num_threads = std::thread::hardware_concurrency();
  num_threads = std::min(num_threads, ues.size());

  std::vector<std::thread> threads;
  size_t chunk_size = (ues.size() + num_threads - 1) / num_threads;

  std::vector<cf_t> source = std::vector<cf_t>(ptr_rx->begin(), ptr_rx->begin() + gnb_ns);
  
  for(size_t t = 0; t < num_threads; t++){
    size_t start = t * chunk_size;
    size_t end = std::min(start + chunk_size, ues.size());

    threads.emplace_back([&](size_t s, size_t e) {
      std::vector<cf_t> scaled(gnb_ns);
      for(size_t i = s; i < e; i++){
        auto& ue = ues[i];
        std::copy(source.begin(), source.end(), scaled.begin());
        multiply_by_const(scaled, ue.pl);
        // add_vectors_volk(scaled, noise);
        ring_push(ue.dl_ring, scaled.data(), gnb_ns);
        
      }
    }, start, end);
  }

  for (auto& th : threads) th.join();
}


int main(int argc, char* argv[]){
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
 
    sigaction(SIGINT, &sigIntHandler, NULL);

    // size_t ue_num = 4;
    size_t ue_num = atoi(argv[1]);

    //initialize ports

    const int port_gnb_tx = 2000;
    const int port_gnb_rx = 2001;

    const std::vector<int> ue_tx_ports{2111, 2121, 2131, 2141, 2151, 2161, 2171, 2101, 2201, 2301};
    const std::vector<int> ue_rx_ports{2110, 2120, 2130, 2140, 2150, 2160, 2170, 2100, 2200, 2300};

    std::string addr_recv_port_gnb_tx = "tcp://localhost:" + std::to_string(port_gnb_tx);
    std::string addr_recv_port_gnb_rx = "tcp://*:" + std::to_string(port_gnb_rx);

    int ret = 0;

    void *context = zmq_ctx_new ();

    Req gnb_dl;
    Rep gnb_ul;

    gnb_dl.sock = zmq_socket(context, ZMQ_REQ);
    gnb_ul.sock = zmq_socket(context, ZMQ_REP);

    set_sockopts(gnb_dl.sock); 
    set_sockopts(gnb_ul.sock);

    if (!gnb_dl.sock || !gnb_ul.sock) {
      printf("Failed to create gNB sockets\n");
    return 1;
  }

  if (zmq_connect(gnb_dl.sock, addr_recv_port_gnb_tx.c_str()) != 0 || zmq_bind(gnb_ul.sock, addr_recv_port_gnb_rx.c_str()) != 0) {
    printf("gNB connect or bind failed\n");
    return 1;
  }

    // UL socket work

    // CHANNEL MODER
    // std::vector<float> pathloss = {0.0f, 10.0f, 20.0f, 20.0f, 20.0f, 20.0f};
    std::vector<double> distance = {0.05f, 0.1f, 0.15f, 0.2f, 0.25f, 0.35f};
    
    int N_0 = -100;
    std::vector<cf_t> noise = WGN(SAMPLES_PER_SLOT, N_0);


    std::vector<UePath> ues;
    for(size_t i = 0; i < ue_num; i++){
      UePath ue;

      double pl = PL_coeff(distance[i]);
      ue.pl = db_to_pl(pl);

      ue.ul_req.sock = zmq_socket(context, ZMQ_REQ);
      ue.dl_rep.sock = zmq_socket(context, ZMQ_REP);
      if (!ue.ul_req.sock || !ue.dl_rep.sock || !set_sockopts(ue.ul_req.sock) || !set_sockopts(ue.dl_rep.sock)) {
        printf("Failed to create UE[%d] sockets\n", i);
        return 1;
      }

      std::string tx_addr = "tcp://127.0.0.1:" + std::to_string(ue_tx_ports[i]);
      std::string rx_addr = "tcp://*:" + std::to_string(ue_rx_ports[i]);
      if (zmq_connect(ue.ul_req.sock, tx_addr.c_str()) != 0 || zmq_bind(ue.dl_rep.sock, rx_addr.c_str()) != 0) {
        printf("UE[%d] connect/bind failed\n", i);
        return 1;
      }

      ues.emplace_back(std::move(ue));
    }


  std::vector<cf_t> rx_tmp(MAX_PULL_SAMPLES);
  std::vector<cf_t> slot_buf(SAMPLES_PER_SLOT);
  std::vector<cf_t> dl_slot(SAMPLES_PER_SLOT);
  std::vector<cf_t> ul_slot(SAMPLES_PER_SLOT);
  printf("Broker started\n");

  int cycle = 0;

  while(1){
    // ask data from gNB and if got smth -> push to ue ring buffer (dl)
    int gnb_ns = req_try_pull(gnb_dl, rx_tmp);
    if (gnb_ns < 0) {
      printf("gNB DL req error\n");
      break;
    }
    if (gnb_ns > 0) {
      parallel_push(ues, &rx_tmp, noise, gnb_ns);
    }

    // ask data from UE and if got smth -> push to ue ring buffer (ul)
    for (auto& ue : ues) {
      int ue_ns = req_try_pull(ue.ul_req, rx_tmp);
      if(ue_ns < 0){
        printf("UE req error\n");
        break;
      }
      if(ue_ns > 0) {
        ring_push(ue.ul_ring, rx_tmp.data(), ue_ns);
      }
    }

    // gNB request data 
    if (rep_try_recv_request(gnb_ul) < 0) {
      printf("gNB UL rep recv error \n");
      break;
    }

    // UEs request data
    for (auto& ue : ues) {
      if (rep_try_recv_request(ue.dl_rep) < 0) {
        printf("UE DL rep recv error\n");
        break;
      }
    }

    //DL
    for (auto& ue : ues) {
      if (!ue.dl_rep.request_pending) {
        continue;
      }
      
      int req_ns = ue.dl_rep.requested_samples;
      if (!ring_pop_exact(ue.dl_ring, dl_slot, req_ns)) {
        continue;
      }
      if (rep_send_response(ue.dl_rep, dl_slot.data(), dl_slot.size()) < 0) {
        printf("UE DL rep send error\n");
        break;
      }
    }

    //UL
    if (gnb_ul.request_pending) {
      int req_ns = gnb_ul.requested_samples;
      bool all_ready = true;
      for (auto& ue : ues) {
        if (ue.ul_ring.size() < req_ns) {
          all_ready = false;
          break;
        }
      }
      if (all_ready) {
        ul_slot.assign(req_ns, cf_t(0.0f, 0.0f));
        parallel_pull(ues, req_ns, noise);
        for (auto& ue : ues) {
          // slot_buf.resize(req_ns);
          // ring_pop_exact(ue.ul_ring, slot_buf, req_ns);
          // multiply_by_const(slot_buf, ue.pl);
          // add_vectors_volk(slot_buf, noise);
          add_vectors_volk(ul_slot, ue.slot_buf);
        }
        // add_vectors_volk(ul_slot, noise);
        if (rep_send_response(gnb_ul, ul_slot.data(), ul_slot.size()) < 0) {
          printf("gNB UL rep send error\n");
          break;
        }
      }
    }

    // if (cycle == 1000) {
    //   std::printf("gnb_rx=%d gnb_tx=%d", gnb_dl.rx_samples, gnb_ul.tx_samples);
    //   for (size_t i = 0; i < ues.size(); ++i) {
    //     std::printf("ue%d_rx=%lu ue%d_tx=%d \n", i + 1, ues[i].ul_req.rx_samples, i + 1, ues[i].dl_rep.tx_samples);
    //   }
    //   // std::printf("\n");
    //   cycle = 0;
    // }
    // cycle++;

    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }

  zmq_close(gnb_ul.sock);
  zmq_close(gnb_dl.sock);
  for (auto& ue : ues) {
    zmq_close(ue.dl_rep.sock);
    zmq_close(ue.ul_req.sock);
  }
  zmq_ctx_destroy(context);
  std::printf("Broker stopped\n");
  return 0;
}
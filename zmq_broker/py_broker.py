#!/usr/bin/env python3
"""
ZMQ Radio Broker for srsRAN 4G/5G system-level simulation with integrated
3GPP TR 38.901 radio channel models using NVIDIA Sionna (with NumPy fallback).

Replaces the C++ broker and external MATLAB/Python channel engine into
a single, unified, high-performance Python application.
"""

import sys
import time
import argparse
import struct
from collections import deque
import yaml
import zmq
import numpy as np

# Try importing NVIDIA Sionna and TensorFlow
try:
    import tensorflow as tf
    # Configure TensorFlow to allocate GPU memory dynamically if available
    gpus = tf.config.experimental.list_physical_devices('GPU')
    for gpu in gpus:
        tf.config.experimental.set_memory_growth(gpu, True)
    from sionna.channel.tr38901 import TDL
    HAS_SIONNA = True
except (ImportError, Exception):
    HAS_SIONNA = False


# =====================================================================
# 3GPP TR 38.901 Channel Implementations
# =====================================================================

class NumPyTDLChannel:
    """
    Pure NumPy implementation of 3GPP TR 38.901 TDL-C (NLOS) channel.
    Maintains phase continuity across slots.
    """
    def __init__(self, sample_rate=11.52e6, carrier_freq=900e6, delay_spread=100e-9, max_doppler=15.0):
        self.fs = float(sample_rate)
        self.fc = float(carrier_freq)
        self.ds = float(delay_spread)
        self.fd = float(max_doppler)

        # 3GPP TR 38.901 Table 7.7.2 TDL-C profile (normalized delay tau / DS, power dB)
        self.tau_norm = np.array([0.0, 0.2099, 0.2219, 0.4354, 0.5057, 0.6015,
                                  0.8166, 1.2520, 1.4883, 2.0722, 2.6582, 3.4214], dtype=np.float32)
        self.p_db = np.array([-4.4, -1.2, -3.5, -5.2, -2.5, 0.0,
                              -2.2, -3.4, -3.4, -7.4, -7.4, -8.4], dtype=np.float32)

        self.p_lin = 10.0 ** (self.p_db / 20.0)
        self.p_lin /= np.sqrt(np.sum(self.p_lin**2))

        delays_sec = self.tau_norm * self.ds
        self.delay_samples = np.round(delays_sec * self.fs).astype(int)
        self.num_taps = len(self.p_db)
        self.phases = np.random.uniform(0, 2 * np.pi, self.num_taps).astype(np.float32)

    def filter(self, x: np.ndarray) -> np.ndarray:
        N = len(x)
        y = np.zeros(N, dtype=np.complex64)
        t = (np.arange(N, dtype=np.float32)) / self.fs

        for i in range(self.num_taps):
            d = self.delay_samples[i]
            phase_t = 2.0 * np.pi * self.fd * t + self.phases[i]
            fading = self.p_lin[i] * (np.cos(phase_t) + 1j * np.sin(phase_t))

            if d == 0:
                y += x * fading
            elif d < N:
                y[d:] += x[:-d] * fading[d:]

            self.phases[i] = np.fmod(self.phases[i] + 2.0 * np.pi * self.fd * (N / self.fs), 2.0 * np.pi)

        return y


class SionnaTDLChannel:
    """
    Channel wrapper leveraging NVIDIA Sionna 3GPP TR 38.901 TDL model,
    with automatic fallback to NumPyTDLChannel if Sionna is unavailable.
    """
    def __init__(self, model="C", sample_rate=11.52e6, carrier_freq=900e6,
                 delay_spread=100e-9, speed_kmh=15.0):
        self.sample_rate = float(sample_rate)
        self.carrier_freq = float(carrier_freq)
        self.speed_ms = (float(speed_kmh) / 3.6)
        self.max_doppler = self.speed_ms * (self.carrier_freq / 3e8)

        if HAS_SIONNA:
            try:
                self.channel = TDL(
                    model=model,
                    delay_spread=delay_spread,
                    carrier_frequency=carrier_freq,
                    min_speed=self.speed_ms,
                    max_speed=self.speed_ms
                )
                self.backend = "Sionna (TensorFlow)"
            except Exception as e:
                print(f"[Warning] Failed to initialize Sionna TDL: {e}. Falling back to NumPy.")
                self.channel = NumPyTDLChannel(sample_rate, carrier_freq, delay_spread, self.max_doppler)
                self.backend = "NumPy (3GPP TDL-C)"
        else:
            self.channel = NumPyTDLChannel(sample_rate, carrier_freq, delay_spread, self.max_doppler)
            self.backend = "NumPy (3GPP TDL-C)"

    def apply(self, x: np.ndarray, distance_km: float, snr_db: float = 30.0) -> np.ndarray:
        N = len(x)
        if N == 0:
            return x

        # 1. Multi-path fading (TDL)
        if HAS_SIONNA and self.backend.startswith("Sionna"):
            x_tf = tf.constant(x.reshape(1, 1, 1, N), dtype=tf.complex64)
            y_tf = self.channel(x_tf, sampling_frequency=self.sample_rate)
            y = y_tf.numpy().reshape(-1)
        else:
            y = self.channel.filter(x)

        # 2. Path loss (3GPP Free Space / Urban Macro)
        # PL (dB) = 32.44 + 20*log10(fc_MHz) + 20*log10(d_km)
        fc_mhz = self.carrier_freq / 1e6
        d_km = max(float(distance_km), 0.001)
        pl_db = 32.44 + 20.0 * np.log10(fc_mhz) + 20.0 * np.log10(d_km)
        pl_linear = 10.0 ** (-pl_db / 20.0)
        y = y * pl_linear

        # 3. Additive White Gaussian Noise (AWGN)
        if snr_db is not None and snr_db > 0:
            sig_pwr = np.mean(np.abs(y)**2)
            if sig_pwr > 1e-12:
                noise_pwr = sig_pwr / (10.0 ** (snr_db / 10.0))
                noise = (np.random.randn(N) + 1j * np.random.randn(N)) * np.sqrt(noise_pwr / 2.0)
                y = y + noise

        return y.astype(np.complex64)


# =====================================================================
# Radio Endpoint Class (srsRAN ZeroMQ Protocol)
# =====================================================================

class RadioEndpoint:
    SAMPLES_PER_SLOT = 11520
    MAX_PULL_SAMPLES = 11520 * 8
    RING_CAPACITY    = 11520 * 400

    def __init__(self, zmq_ctx, endpoint_id: int, tx_pull_addr: str, rx_rep_addr: str, distance_km: float):
        self.id = endpoint_id
        self.distance_km = distance_km
        self.tx_pull_addr = tx_pull_addr
        self.rx_rep_addr = rx_rep_addr

        # REQ socket to pull TX samples from entity (gNB or UE)
        self.sock_pull = zmq_ctx.socket(zmq.REQ)
        self.sock_pull.setsockopt(zmq.RCVTIMEO, 0)
        self.sock_pull.setsockopt(zmq.SNDTIMEO, 0)
        self.sock_pull.setsockopt(zmq.RCVHWM, 2000)
        self.sock_pull.setsockopt(zmq.SNDHWM, 2000)
        self.sock_pull.connect(tx_pull_addr)

        # REP socket to serve RX samples to entity (gNB or UE)
        self.sock_rep = zmq_ctx.socket(zmq.REP)
        self.sock_rep.setsockopt(zmq.RCVTIMEO, 0)
        self.sock_rep.setsockopt(zmq.SNDTIMEO, 0)
        self.sock_rep.setsockopt(zmq.RCVHWM, 2000)
        self.sock_rep.setsockopt(zmq.SNDHWM, 2000)
        self.sock_rep.bind(rx_rep_addr)

        self.req_waiting = False
        self.rep_pending = False
        self.requested_samples = self.SAMPLES_PER_SLOT

        self.tx_ring = deque(maxlen=self.RING_CAPACITY)
        self.rx_ring = deque(maxlen=self.RING_CAPACITY)

    def poll_tx(self):
        """Polls transmission samples from srsRAN node via REQ/REP."""
        if not self.req_waiting:
            try:
                # Ask srsRAN node for MAX_PULL_SAMPLES
                ask = struct.pack("=i", self.MAX_PULL_SAMPLES)
                self.sock_pull.send(ask, flags=zmq.NOBLOCK)
                self.req_waiting = True
            except zmq.ZMQError:
                return 0

        try:
            raw_bytes = self.sock_pull.recv(flags=zmq.NOBLOCK)
            self.req_waiting = False
            samples = np.frombuffer(raw_bytes, dtype=np.complex64)
            if len(samples) > 0:
                self.tx_ring.extend(samples)
            return len(samples)
        except zmq.ZMQError:
            return 0

    def poll_rx_request(self):
        """Checks if srsRAN node requested RX samples via REP socket."""
        if self.rep_pending:
            return

        try:
            req_data = self.sock_rep.recv(flags=zmq.NOBLOCK)
            if len(req_data) >= 4:
                ask_count = struct.unpack("=i", req_data[:4])[0]
                self.requested_samples = ask_count if ask_count > 0 else self.SAMPLES_PER_SLOT
            else:
                self.requested_samples = self.SAMPLES_PER_SLOT
            self.rep_pending = True
        except zmq.ZMQError:
            pass

    def flush_rx_response(self) -> bool:
        """Sends requested RX samples to srsRAN node."""
        if not self.rep_pending:
            return False
        if len(self.rx_ring) < self.requested_samples:
            return False

        # Pop required number of samples
        out = np.empty(self.requested_samples, dtype=np.complex64)
        for i in range(self.requested_samples):
            out[i] = self.rx_ring.popleft()

        try:
            self.sock_rep.send(out.tobytes(), flags=zmq.NOBLOCK)
            self.rep_pending = False
            return True
        except zmq.ZMQError:
            return False

    def push_rx_samples(self, samples: np.ndarray):
        self.rx_ring.extend(samples)

    def has_tx_samples(self, n: int) -> bool:
        return len(self.tx_ring) >= n

    def pop_tx_samples(self, n: int) -> np.ndarray:
        out = np.empty(n, dtype=np.complex64)
        for i in range(n):
            out[i] = self.tx_ring.popleft()
        return out


# =====================================================================
# Main Python Radio Broker with Sionna
# =====================================================================

class PyZmqRadioBroker:
    def __init__(self, config_path: str):
        with open(config_path, "r") as f:
            self.config = yaml.safe_load(f)

        self.zmq_ctx = zmq.Context()

        # Channel parameters
        ch_cfg = self.config.get("channel", {})
        self.carrier_freq = float(ch_cfg.get("carrier_freq_mhz", 900.0)) * 1e6
        self.sample_rate = 11.52e6

        # Initialize Base Station (gNodeB)
        gnb_cfg = self.config["gnb"]
        gnb_tx_addr = f"tcp://127.0.0.1:{gnb_cfg['tx_port']}"
        gnb_rx_addr = f"tcp://127.0.0.1:{gnb_cfg['rx_port']}"
        self.gnb = RadioEndpoint(self.zmq_ctx, -1, gnb_tx_addr, gnb_rx_addr, 0.0)

        # Initialize User Equipments (UEs) and individual 3GPP channels
        self.ues = []
        self.dl_channels = {}
        self.ul_channels = {}

        ues_cfg = self.config.get("ues", [])
        for u in ues_cfg:
            ue_id = u["id"]
            ue_tx_addr = f"tcp://127.0.0.1:{u['tx_port']}"
            ue_rx_addr = f"tcp://127.0.0.1:{u['rx_port']}"
            dist_km = float(u.get("distance_km", 0.1))
            speed = float(u.get("speed_kmh", 15.0))

            ep = RadioEndpoint(self.zmq_ctx, ue_id, ue_tx_addr, ue_rx_addr, dist_km)
            self.ues.append(ep)

            # Separate channels for Downlink and Uplink
            self.dl_channels[ue_id] = SionnaTDLChannel(
                model="C",
                sample_rate=self.sample_rate,
                carrier_freq=self.carrier_freq,
                delay_spread=100e-9,
                speed_kmh=speed
            )
            self.ul_channels[ue_id] = SionnaTDLChannel(
                model="C",
                sample_rate=self.sample_rate,
                carrier_freq=self.carrier_freq,
                delay_spread=100e-9,
                speed_kmh=speed
            )

        backend_name = self.dl_channels[0].backend if self.dl_channels else "None"
        print(f"=======================================================")
        print(f"  PyZMQ 3GPP Radio Broker (srsRAN 4G/5G)")
        print(f"  Active UEs: {len(self.ues)}")
        print(f"  Carrier Frequency: {self.carrier_freq / 1e6:.1f} MHz")
        print(f"  Channel Engine: {backend_name}")
        print(f"=======================================================")

    def process_downlink(self):
        """Distributes Downlink slots from gNodeB to all active UEs through 3GPP channels."""
        if self.gnb.has_tx_samples(RadioEndpoint.SAMPLES_PER_SLOT):
            raw_dl_slot = self.gnb.pop_tx_samples(RadioEndpoint.SAMPLES_PER_SLOT)

            for ue in self.ues:
                # Apply 3GPP channel (TDL-C fading + path loss + noise)
                faded_dl = self.dl_channels[ue.id].apply(raw_dl_slot, ue.distance_km)
                ue.push_rx_samples(faded_dl)

        for ue in self.ues:
            ue.flush_rx_response()

    def accumulate_uplink(self) -> bool:
        """Collects Uplink slots from all UEs, filters them, and sums them into gNodeB."""
        if not self.gnb.rep_pending:
            return False

        slot_size = self.gnb.requested_samples

        # Check if all UEs have accumulated enough samples
        for ue in self.ues:
            if not ue.has_tx_samples(slot_size):
                return False

        combined_ul = np.zeros(slot_size, dtype=np.complex64)

        for ue in self.ues:
            raw_ul = ue.pop_tx_samples(slot_size)
            # Apply individual 3GPP Uplink channel
            faded_ul = self.ul_channels[ue.id].apply(raw_ul, ue.distance_km)
            combined_ul += faded_ul

        self.gnb.push_rx_samples(combined_ul)
        return self.gnb.flush_rx_response()

    def run(self):
        print("[Broker] Running main loop. Press Ctrl+C to stop.")
        cycle_count = 0
        last_stat = time.time()

        try:
            while True:
                # 1. Poll TX data from all endpoints
                self.gnb.poll_tx()
                for ue in self.ues:
                    ue.poll_tx()

                # 2. Poll RX requests
                self.gnb.poll_rx_request()
                for ue in self.ues:
                    ue.poll_rx_request()

                # 3. Process Downlink and Uplink streams
                self.process_downlink()
                self.accumulate_uplink()

                cycle_count += 1
                if cycle_count % 10000 == 0:
                    now = time.time()
                    dt = now - last_stat
                    if dt > 3.0:
                        print(f"[Broker Status] Processed {cycle_count} cycles. DL buffer: {len(self.gnb.tx_ring)} samples")
                        last_stat = now
                    time.sleep(0.0001)

        except KeyboardInterrupt:
            print("\n[Broker] Terminating by user.")


def main():
    parser = argparse.ArgumentParser(description="Python ZMQ Radio Broker with 3GPP Sionna Channel")
    parser.add_argument("config", nargs="?", default="config-v2.yaml", help="Path to config.yaml")
    args = parser.parse_args()

    broker = PyZmqRadioBroker(args.config)
    broker.run()


if __name__ == "__main__":
    main()

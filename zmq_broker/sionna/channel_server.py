#!/usr/bin/env python3
"""
channel_server.py - High-Performance 3GPP TR 38.901 Radio Channel Server in Python
Replaces MATLAB Channel Server for ZeroMQ-based telecommunication system-level simulation.

Protocol:
  ZMQ_REP socket on tcp://*:5555
  Request:
    Frame 1: 16 bytes header (int32 ue_id, int32 direction, float32 distance_km, int32 num_samples)
    Frame 2: raw float32 IQ samples [I0, Q0, I1, Q1, ...] (num_samples * 8 bytes)
  Response:
    Frame 1: processed raw float32 IQ samples (num_samples * 8 bytes)
"""

import sys
import struct
import signal
import numpy as np
import zmq

class TDLChannel3GPP:
    """
    3GPP TR 38.901 Tapped Delay Line (TDL-C) Channel Model with phase memory.
    """
    def __init__(self, sample_rate=11.52e6, carrier_freq=900e6,
                 delay_spread=100e-9, max_doppler=15.0):
        self.fs = sample_rate
        self.fc = carrier_freq
        self.ds = delay_spread
        self.fd = max_doppler

        # 3GPP TR 38.901 Table 7.7.2-3 (TDL-C model, 12-tap simplified)
        # Normalized delays (tau / DS)
        self.tau_norm = np.array([
            0.0000, 0.2099, 0.2219, 0.4354, 0.5057, 0.6015,
            0.8166, 1.2520, 1.4883, 2.0722, 2.6582, 3.4214
        ], dtype=np.float32)

        # Average power in dB
        self.p_db = np.array([
            -4.4, -1.2, -3.5, -5.2, -2.5,  0.0,
            -2.2, -3.4, -3.4, -7.4, -7.4, -8.4
        ], dtype=np.float32)

        # Linear amplitude normalized to unit power
        self.p_lin = 10.0 ** (self.p_db / 20.0)
        self.p_lin /= np.sqrt(np.sum(self.p_lin ** 2))

        # Sample delays (quantized to sample rate)
        delays_sec = self.tau_norm * self.ds
        self.delay_samples = np.round(delays_sec * self.fs).astype(int)

        self.num_taps = len(self.p_db)
        # Random initial phases for each tap
        self.phases = np.random.uniform(0.0, 2.0 * np.pi, self.num_taps).astype(np.float32)

    def filter(self, x: np.ndarray, distance_km: float, snr_db: float = 30.0) -> np.ndarray:
        n_samples = len(x)
        y = np.zeros(n_samples, dtype=np.complex64)
        t = (np.arange(n_samples, dtype=np.float32) / self.fs)

        # 1. Multi-tap Multipath Fading with Doppler Shift
        for i in range(self.num_taps):
            d = self.delay_samples[i]
            # Time-variant Doppler phase rotation
            doppler_phase = 2.0 * np.pi * self.fd * t + self.phases[i]
            fading_coef = self.p_lin[i] * np.exp(1j * doppler_phase)

            if d == 0:
                y += x * fading_coef
            elif d < n_samples:
                delayed_x = np.pad(x[:-d], (d, 0), mode='constant')
                y += delayed_x * fading_coef

            # Update accumulated phase to preserve continuity across consecutive slots
            self.phases[i] = np.fmod(self.phases[i] + 2.0 * np.pi * self.fd * (n_samples / self.fs), 2.0 * np.pi)

        # 2. 3GPP Path Loss (Free Space / Urban Macro approximation)
        # PL (dB) = 32.44 + 20*log10(fc_MHz) + 20*log10(d_km)
        fc_mhz = self.fc / 1e6
        safe_dist = max(distance_km, 0.001)  # avoid log10(0)
        pl_db = 32.44 + 20.0 * np.log10(fc_mhz) + 20.0 * np.log10(safe_dist)
        pl_linear = 10.0 ** (-pl_db / 20.0)
        y *= pl_linear

        # 3. Additive White Gaussian Noise (AWGN)
        if snr_db is not None:
            snr_linear = 10.0 ** (snr_db / 10.0)
            sig_power = np.mean(np.abs(y) ** 2)
            noise_power = sig_power / snr_linear if sig_power > 1e-12 else 1e-12
            noise = (np.random.randn(n_samples) + 1j * np.random.randn(n_samples)) * np.sqrt(noise_power / 2.0)
            y += noise

        return y.astype(np.complex64)


def run_server(bind_endpoint: str = "tcp://*:5555"):
    context = zmq.Context()
    socket = context.socket(zmq.REP)
    socket.bind(bind_endpoint)
    print(f"[Python 3GPP Server] Listening on {bind_endpoint} (Ctrl+C to stop)...")

    # Storage for per-UE channel instances (preserves channel state and fading across slots)
    channels = {}
    HEADER_STRUCT = struct.Struct("<iifi")  # int32 ue_id, int32 direction, float32 distance_km, int32 num_samples
    HEADER_SIZE = HEADER_STRUCT.size

    def signal_handler(sig, frame):
        print("\n[Python 3GPP Server] Stopping server...")
        socket.close()
        context.term()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    slot_count = 0

    while True:
        try:
            # 1. Receive Header (Frame 1)
            hdr_bytes = socket.recv()
            if len(hdr_bytes) != HEADER_SIZE:
                print(f"[Python 3GPP Server] Warning: Invalid header size: {len(hdr_bytes)} bytes (expected {HEADER_SIZE})")
                socket.send(b"")
                continue

            ue_id, direction, distance_km, num_samples = HEADER_STRUCT.unpack(hdr_bytes)

            # 2. Receive IQ Samples (Frame 2)
            samples_bytes = socket.recv()
            expected_bytes = num_samples * 8  # 2 x float32 per complex sample
            if len(samples_bytes) != expected_bytes:
                print(f"[Python 3GPP Server] Warning: Incomplete payload: got {len(samples_bytes)} bytes, expected {expected_bytes}")
                socket.send(b"")
                continue

            # Convert binary buffer into numpy array of complex numbers
            raw_floats = np.frombuffer(samples_bytes, dtype=np.float32)
            iq_in = raw_floats[0::2] + 1j * raw_floats[1::2]

            # Unique key for channel: (ue_id, direction) to keep DL and UL states separate
            channel_key = (ue_id, direction)
            if channel_key not in channels:
                dir_str = "DL" if direction == 0 else "UL"
                print(f"[Python 3GPP Server] Initialized TDL-C channel for UE {ue_id} ({dir_str})")
                channels[channel_key] = TDLChannel3GPP(
                    sample_rate=11.52e6,
                    carrier_freq=900e6,
                    delay_spread=100e-9,
                    max_doppler=15.0
                )

            channel = channels[channel_key]

            # Filter signal through 3GPP model
            iq_out = channel.filter(iq_in, distance_km=distance_km)

            # Pack processed complex array back into interleaved float32 [I0, Q0, I1, Q1, ...]
            out_floats = np.empty(num_samples * 2, dtype=np.float32)
            out_floats[0::2] = np.real(iq_out)
            out_floats[1::2] = np.imag(iq_out)

            # Send single-frame response to C++ broker
            socket.send(out_floats.tobytes())

            slot_count += 1
            if slot_count % 5000 == 0:
                print(f"[Python 3GPP Server] Processed {slot_count} slots successfully.")

        except zmq.ZMQError as e:
            if e.errno == zmq.ETERM:
                break
            print(f"[Python 3GPP Server] ZMQ Error: {e}")
        except Exception as e:
            print(f"[Python 3GPP Server] Exception: {e}")


if __name__ == "__main__":
    endpoint = sys.argv[1] if len(sys.argv) > 1 else "tcp://*:5555"
    run_server(endpoint)

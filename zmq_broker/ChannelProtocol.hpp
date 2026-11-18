#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct ChannelRequestHeader {
    int32_t ue_id;        // UE identifier (0, 1, 2...)
    int32_t direction;    // 0 = Downlink (gNB -> UE), 1 = Uplink (UE -> gNB)
    float   distance_km;  // Current distance in km
    int32_t num_samples;  // Number of complex samples per slot (e.g. 11520)
};
#pragma pack(pop)

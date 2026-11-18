#pragma once

#include <string>
#include <vector>
#include <cmath>
#include <yaml-cpp/yaml.h>

struct UeConfig {
    int id = 0;
    int tx_port = 0;
    int rx_port = 0;
    double distance_km = 0.1;
    float path_loss_lin = 1.0f;
};

struct ChannelConfig {
    double freq_mhz = 900.0;
    double bs_height_m = 45.0;
    double ue_height_m = 1.5;
};

struct BrokerConfig {
    bool matlab_enabled = false;
    std::string matlab_endpoint = "tcp://127.0.0.1:5555";
    ChannelConfig channel;
    int gnb_tx_port = 2000;
    int gnb_rx_port = 2001;
    std::vector<UeConfig> ues;

    static double db_to_linear(double db) {
        return std::pow(10.0, -db / 20.0);
    }

    static double calculate_path_loss_db(double dist_km, const ChannelConfig& ch) {
        constexpr double A = 69.55;
        constexpr double B = 26.16;
        const double f = ch.freq_mhz;
        const double hBs = ch.bs_height_m;
        const double hMs = ch.ue_height_m;

        double a = 1.1 * std::log10(f) * hMs - (1.56 * std::log10(f) - 0.8);
        double Lclutter = -(std::pow(4.78 * std::log10(11.75 * hMs), 2) - 18.33 * std::log10(f) + 40.94);
        double s = 44.9 - 6.55 * std::log10(hBs);
        return A + B * std::log10(f) - 13.82 * std::log10(hBs) - a + s * std::log10(dist_km) + Lclutter;
    }

    static BrokerConfig load(const std::string& filepath) {
        YAML::Node root = YAML::LoadFile(filepath);
        BrokerConfig cfg;

        if (root["matlab_engine"]) {
            cfg.matlab_enabled = root["matlab_engine"]["enabled"].as<bool>(false);
            cfg.matlab_endpoint = root["matlab_engine"]["endpoint"].as<std::string>("tcp://127.0.0.1:5555");
        }

        if (root["channel"]) {
            cfg.channel.freq_mhz = root["channel"]["carrier_freq_mhz"].as<double>(900.0);
            cfg.channel.bs_height_m = root["channel"]["bs_height_m"].as<double>(45.0);
            cfg.channel.ue_height_m = root["channel"]["ue_height_m"].as<double>(1.5);
        }

        if (root["gnb"]) {
            cfg.gnb_tx_port = root["gnb"]["tx_port"].as<int>(2000);
            cfg.gnb_rx_port = root["gnb"]["rx_port"].as<int>(2001);
        }

        if (root["ues"]) {
            for (const auto& node : root["ues"]) {
                UeConfig ue;
                ue.id = node["id"].as<int>();
                ue.tx_port = node["tx_port"].as<int>();
                ue.rx_port = node["rx_port"].as<int>();
                ue.distance_km = node["distance_km"].as<double>(0.1);

                double pl_db = calculate_path_loss_db(ue.distance_km, cfg.channel);
                ue.path_loss_lin = static_cast<float>(db_to_linear(pl_db));

                cfg.ues.push_back(ue);
            }
        }
        return cfg;
    }
};

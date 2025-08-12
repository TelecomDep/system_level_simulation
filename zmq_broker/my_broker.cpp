#include <csignal>
#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/mman.h>
#include <unistd.h>
#include <algorithm>

#include <cstdint>

#include "/usr/local/include/srsran/common/common_helper.h"
#include "/usr/local/include/srsran/common/config_file.h"
#include "/usr/local/include/srsran/common/crash_handler.h"
#include "/usr/local/include/srsran/common/metrics_hub.h"
#include "/usr/local/include/srsran/common/multiqueue.h"
#include "/usr/local/include/srsran/common/tsan_options.h"
#include "/usr/local/include/srsran/srslog/event_trace.h"
#include "/usr/local/include/srsran/srslog/srslog.h"
#include "/usr/local/include/srsran/srsran.h"
#include "/usr/local/include/srsran/support/emergency_handlers.h"
#include "/usr/local/include/srsran/support/signal_handler.h"
#include "/usr/local/include/srsran/version.h"

#include "hdr/metrics_csv.h"
#include "hdr/metrics_json.h"
#include "hdr/metrics_stdout.h"
#include "hdr/ue.h"

#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>


#include <zmq.h>

#include <vector>
#include <random>
#include <complex>
#include <algorithm>
#include <regex>

using namespace srsue;
using namespace std;
namespace bpo = boost::program_options;

#define BUFFER_MAX 1024 * 1024


typedef _Complex float cf_t;
#define NSAMPLES2NBYTES(X) (((uint32_t)(X)) * sizeof(cf_t))
#define NBYTES2NSAMPLES(X) ((X) / sizeof(cf_t))
#define ZMQ_MAX_BUFFER_SIZE (NSAMPLES2NBYTES(3072000)) // 10 subframes at 20 MHz
#define NBYTES_PER_ONE_SAMPLE (NSAMPLES2NBYTES(1)) // 1 sample

bool        use_standard_lte_rates = false;
string scs_khz, ssb_scs_khz;
string cfr_mode;

int ue_parser(all_args_t* args, string config_file){
    bpo::options_description common("Configuration options");

    common.add_options()
    ("ue.radio", bpo::value<string>(&args->rf.type)->default_value("multi"), "Type of the radio [multi]")
    ("ue.phy", bpo::value<string>(&args->phy.type)->default_value("lte"), "Type of the PHY [lte]")

    ("rf.srate",        bpo::value<double>(&args->rf.srate_hz)->default_value(0.0),          "Force Tx and Rx sampling rate in Hz")
    ("rf.freq_offset",  bpo::value<float>(&args->rf.freq_offset)->default_value(0),          "(optional) Frequency offset")
    ("rf.rx_gain",      bpo::value<float>(&args->rf.rx_gain)->default_value(-1),             "Front-end receiver gain")
    ("rf.tx_gain",      bpo::value<float>(&args->rf.tx_gain)->default_value(-1),             "Front-end transmitter gain (all channels)")
    ("rf.tx_gain[0]",   bpo::value<float>(&args->rf.tx_gain_ch[0])->default_value(-1),       "Front-end transmitter gain CH0")
    ("rf.tx_gain[1]",   bpo::value<float>(&args->rf.tx_gain_ch[1])->default_value(-1),       "Front-end transmitter gain CH1")
    ("rf.tx_gain[2]",   bpo::value<float>(&args->rf.tx_gain_ch[2])->default_value(-1),       "Front-end transmitter gain CH2")
    ("rf.tx_gain[3]",   bpo::value<float>(&args->rf.tx_gain_ch[3])->default_value(-1),       "Front-end transmitter gain CH3")
    ("rf.tx_gain[4]",   bpo::value<float>(&args->rf.tx_gain_ch[4])->default_value(-1),       "Front-end transmitter gain CH4")
    ("rf.rx_gain[0]",   bpo::value<float>(&args->rf.rx_gain_ch[0])->default_value(-1),       "Front-end receiver gain CH0")
    ("rf.rx_gain[1]",   bpo::value<float>(&args->rf.rx_gain_ch[1])->default_value(-1),       "Front-end receiver gain CH1")
    ("rf.rx_gain[2]",   bpo::value<float>(&args->rf.rx_gain_ch[2])->default_value(-1),       "Front-end receiver gain CH2")
    ("rf.rx_gain[3]",   bpo::value<float>(&args->rf.rx_gain_ch[3])->default_value(-1),       "Front-end receiver gain CH3")
    ("rf.rx_gain[4]",   bpo::value<float>(&args->rf.rx_gain_ch[4])->default_value(-1),       "Front-end receiver gain CH4")
    ("rf.nof_antennas", bpo::value<uint32_t>(&args->rf.nof_antennas)->default_value(1),      "Number of antennas per carrier")

    ("rf.device_name", bpo::value<string>(&args->rf.device_name)->default_value("auto"), "Front-end device name")
    ("rf.device_args", bpo::value<string>(&args->rf.device_args)->default_value("auto"), "Front-end device arguments")
    ("rf.time_adv_nsamples", bpo::value<string>(&args->rf.time_adv_nsamples)->default_value("auto"), "Transmission time advance")
    ("rf.continuous_tx", bpo::value<string>(&args->rf.continuous_tx)->default_value("auto"), "Transmit samples continuously to the radio or on bursts (auto/yes/no). Default is auto (yes for UHD, no for rest)")

    ("rf.bands.rx[0].min", bpo::value<float>(&args->rf.ch_rx_bands[0].min)->default_value(0), "Lower frequency boundary for CH0-RX")
    ("rf.bands.rx[0].max", bpo::value<float>(&args->rf.ch_rx_bands[0].max)->default_value(0), "Higher frequency boundary for CH0-RX")
    ("rf.bands.rx[1].min", bpo::value<float>(&args->rf.ch_rx_bands[1].min)->default_value(0), "Lower frequency boundary for CH1-RX")
    ("rf.bands.rx[1].max", bpo::value<float>(&args->rf.ch_rx_bands[1].max)->default_value(0), "Higher frequency boundary for CH1-RX")
    ("rf.bands.rx[2].min", bpo::value<float>(&args->rf.ch_rx_bands[2].min)->default_value(0), "Lower frequency boundary for CH2-RX")
    ("rf.bands.rx[2].max", bpo::value<float>(&args->rf.ch_rx_bands[2].max)->default_value(0), "Higher frequency boundary for CH2-RX")
    ("rf.bands.rx[3].min", bpo::value<float>(&args->rf.ch_rx_bands[3].min)->default_value(0), "Lower frequency boundary for CH3-RX")
    ("rf.bands.rx[3].max", bpo::value<float>(&args->rf.ch_rx_bands[3].max)->default_value(0), "Higher frequency boundary for CH3-RX")
    ("rf.bands.rx[4].min", bpo::value<float>(&args->rf.ch_rx_bands[4].min)->default_value(0), "Lower frequency boundary for CH4-RX")
    ("rf.bands.rx[4].max", bpo::value<float>(&args->rf.ch_rx_bands[4].max)->default_value(0), "Higher frequency boundary for CH4-RX")

    ("rf.bands.tx[0].min", bpo::value<float>(&args->rf.ch_tx_bands[0].min)->default_value(0), "Lower frequency boundary for CH1-TX")
    ("rf.bands.tx[0].max", bpo::value<float>(&args->rf.ch_tx_bands[0].max)->default_value(0), "Higher frequency boundary for CH1-TX")
    ("rf.bands.tx[1].min", bpo::value<float>(&args->rf.ch_tx_bands[1].min)->default_value(0), "Lower frequency boundary for CH1-TX")
    ("rf.bands.tx[1].max", bpo::value<float>(&args->rf.ch_tx_bands[1].max)->default_value(0), "Higher frequency boundary for CH1-TX")
    ("rf.bands.tx[2].min", bpo::value<float>(&args->rf.ch_tx_bands[2].min)->default_value(0), "Lower frequency boundary for CH2-TX")
    ("rf.bands.tx[2].max", bpo::value<float>(&args->rf.ch_tx_bands[2].max)->default_value(0), "Higher frequency boundary for CH2-TX")
    ("rf.bands.tx[3].min", bpo::value<float>(&args->rf.ch_tx_bands[3].min)->default_value(0), "Lower frequency boundary for CH3-TX")
    ("rf.bands.tx[3].max", bpo::value<float>(&args->rf.ch_tx_bands[3].max)->default_value(0), "Higher frequency boundary for CH3-TX")
    ("rf.bands.tx[4].min", bpo::value<float>(&args->rf.ch_tx_bands[4].min)->default_value(0), "Lower frequency boundary for CH4-TX")
    ("rf.bands.tx[4].max", bpo::value<float>(&args->rf.ch_tx_bands[4].max)->default_value(0), "Higher frequency boundary for CH4-TX")

    ("rat.eutra.dl_earfcn",    bpo::value<string>(&args->phy.dl_earfcn)->default_value("3400"),     "Downlink EARFCN list")
    ("rat.eutra.ul_earfcn",    bpo::value<string>(&args->phy.ul_earfcn),                            "Uplink EARFCN list. Optional.")
    ("rat.eutra.dl_freq",      bpo::value<float>(&args->phy.dl_freq)->default_value(-1),            "Downlink Frequency (if positive overrides EARFCN)")
    ("rat.eutra.ul_freq",      bpo::value<float>(&args->phy.ul_freq)->default_value(-1),            "Uplink Frequency (if positive overrides EARFCN)")
    ("rat.eutra.nof_carriers", bpo::value<uint32_t>(&args->phy.nof_lte_carriers)->default_value(1), "Number of carriers")
    
    ("rat.nr.bands",        bpo::value<string>(&args->stack.rrc_nr.supported_bands_nr_str)->default_value("3"),   "Supported NR bands")
    ("rat.nr.nof_carriers", bpo::value<uint32_t>(&args->phy.nof_nr_carriers)->default_value(0),                   "Number of NR carriers")
    ("rat.nr.max_nof_prb",  bpo::value<uint32_t>(&args->phy.nr_max_nof_prb)->default_value(52),                   "Maximum NR carrier bandwidth in PRB")
    ("rat.nr.dl_nr_arfcn",  bpo::value<uint32_t>(&args->stack.rrc_nr.dl_nr_arfcn)->default_value(368500),         "DL ARFCN of NR cell")
    ("rat.nr.ssb_nr_arfcn", bpo::value<uint32_t>(&args->stack.rrc_nr.ssb_nr_arfcn)->default_value(368410),        "SSB ARFCN of NR cell")
    ("rat.nr.nof_prb",      bpo::value<uint32_t>(&args->stack.rrc_nr.nof_prb)->default_value(52),                 "Actual NR carrier bandwidth in PRB")
    ("rat.nr.scs",          bpo::value<string>(&scs_khz)->default_value("15"),                                    "PDSCH subcarrier spacing in kHz")
    ("rat.nr.ssb_scs",      bpo::value<string>(&ssb_scs_khz)->default_value("15"),                                "SSB subcarrier spacing in kHz")

    ("rrc.feature_group", bpo::value<uint32_t>(&args->stack.rrc.feature_group)->default_value(0xe6041000),                       "Hex value of the featureGroupIndicators field in the"
                                                                                                                                 "UECapabilityInformation message. Default 0xe6041000")
    ("rrc.ue_category",         bpo::value<string>(&args->stack.rrc.ue_category_str)->default_value(SRSRAN_UE_CATEGORY_DEFAULT),  "UE Category (1 to 10)")
    ("rrc.ue_category_dl",      bpo::value<int>(&args->stack.rrc.ue_category_dl)->default_value(-1),                              "UE Category DL v12 (valid values: 0, 4, 6, 7, 9 to 16)")
    ("rrc.ue_category_ul",      bpo::value<int>(&args->stack.rrc.ue_category_ul)->default_value(-1),                              "UE Category UL v12 (valid values: 0, 3, 5, 7, 8 and 13)")
    ("rrc.release",             bpo::value<uint32_t>(&args->stack.rrc.release)->default_value(SRSRAN_RELEASE_DEFAULT),            "UE Release (8 to 15)")
    ("rrc.mbms_service_id",     bpo::value<int32_t>(&args->stack.rrc.mbms_service_id)->default_value(-1),                         "MBMS service id for autostart (-1 means disabled)")
    ("rrc.mbms_service_port",   bpo::value<uint32_t>(&args->stack.rrc.mbms_service_port)->default_value(4321),                    "Port of the MBMS service")
    ("rrc.nr_measurement_pci",  bpo::value<uint32_t>(&args->stack.rrc_nr.sim_nr_meas_pci)->default_value(500),                    "NR PCI for the simulated NR measurement")
    ("rrc.nr_short_sn_support", bpo::value<bool>(&args->stack.rrc_nr.pdcp_short_sn_support)->default_value(true),                 "Announce PDCP short SN support")

    ("nas.apn",               bpo::value<string>(&args->stack.nas.apn_name)->default_value(""),          "Set Access Point Name (APN) for data services")
    ("nas.apn_protocol",      bpo::value<string>(&args->stack.nas.apn_protocol)->default_value(""),  "Set Access Point Name (APN) protocol for data services")
    ("nas.user",              bpo::value<string>(&args->stack.nas.apn_user)->default_value(""),  "Username for CHAP authentication")
    ("nas.pass",              bpo::value<string>(&args->stack.nas.apn_pass)->default_value(""),  "Password for CHAP authentication")
    ("nas.force_imsi_attach", bpo::value<bool>(&args->stack.nas.force_imsi_attach)->default_value(false),  "Whether to always perform an IMSI attach")
    ("nas.eia",               bpo::value<string>(&args->stack.nas.eia)->default_value("1,2,3"),  "List of integrity algorithms included in UE capabilities")
    ("nas.eea",               bpo::value<string>(&args->stack.nas.eea)->default_value("0,1,2,3"),  "List of ciphering algorithms included in UE capabilities")

    ("slicing.enable",        bpo::value<bool>(&args->stack.nas_5g.enable_slicing)->default_value(false),  "enable slicing in the UE")
    ("slicing.nssai-sst",     bpo::value<int>(&args->stack.nas_5g.nssai_sst)->default_value(1),  "sst of requested slice")
    ("slicing.nssai-sd",      bpo::value<int>(&args->stack.nas_5g.nssai_sd)->default_value(1),  "sd of requested slice")

    ("pcap.enable", bpo::value<string>(&args->stack.pkt_trace.enable)->default_value("none"), "Enable (MAC, MAC_NR, NAS) packet captures for wireshark")
    ("pcap.mac_filename", bpo::value<string>(&args->stack.pkt_trace.mac_pcap.filename)->default_value("/tmp/ue_mac.pcap"), "MAC layer capture filename")
    ("pcap.mac_nr_filename", bpo::value<string>(&args->stack.pkt_trace.mac_nr_pcap.filename)->default_value("/tmp/ue_mac_nr.pcap"), "MAC_NR layer capture filename")
    ("pcap.nas_filename", bpo::value<string>(&args->stack.pkt_trace.nas_pcap.filename)->default_value("/tmp/ue_nas.pcap"), "NAS layer capture filename")
    
    ("gui.enable", bpo::value<bool>(&args->gui.enable)->default_value(false), "Enable GUI plots")

    ("log.rf_level", bpo::value<string>(&args->rf.log_level), "RF log level")
    ("log.phy_level", bpo::value<string>(&args->phy.log.phy_level), "PHY log level")
    ("log.phy_lib_level", bpo::value<string>(&args->phy.log.phy_lib_level), "PHY lib log level")
    ("log.phy_hex_limit", bpo::value<int>(&args->phy.log.phy_hex_limit), "PHY log hex dump limit")
    ("log.mac_level", bpo::value<string>(&args->stack.log.mac_level), "MAC log level")
    ("log.mac_hex_limit", bpo::value<int>(&args->stack.log.mac_hex_limit), "MAC log hex dump limit")
    ("log.rlc_level", bpo::value<string>(&args->stack.log.rlc_level), "RLC log level")
    ("log.rlc_hex_limit", bpo::value<int>(&args->stack.log.rlc_hex_limit), "RLC log hex dump limit")
    ("log.pdcp_level", bpo::value<string>(&args->stack.log.pdcp_level), "PDCP log level")
    ("log.pdcp_hex_limit", bpo::value<int>(&args->stack.log.pdcp_hex_limit), "PDCP log hex dump limit")
    ("log.rrc_level", bpo::value<string>(&args->stack.log.rrc_level), "RRC log level")
    ("log.rrc_hex_limit", bpo::value<int>(&args->stack.log.rrc_hex_limit), "RRC log hex dump limit")
    ("log.gw_level", bpo::value<string>(&args->gw.log.gw_level), "GW log level")
    ("log.gw_hex_limit", bpo::value<int>(&args->gw.log.gw_hex_limit), "GW log hex dump limit")
    ("log.nas_level", bpo::value<string>(&args->stack.log.nas_level), "NAS log level")
    ("log.nas_hex_limit", bpo::value<int>(&args->stack.log.nas_hex_limit), "NAS log hex dump limit")
    ("log.usim_level", bpo::value<string>(&args->stack.log.usim_level), "USIM log level")
    ("log.usim_hex_limit", bpo::value<int>(&args->stack.log.usim_hex_limit), "USIM log hex dump limit")
    ("log.stack_level", bpo::value<string>(&args->stack.log.stack_level), "Stack log level")
    ("log.stack_hex_limit", bpo::value<int>(&args->stack.log.stack_hex_limit), "Stack log hex dump limit")

    ("log.all_level", bpo::value<string>(&args->log.all_level)->default_value("info"), "ALL log level")
    ("log.all_hex_limit", bpo::value<int>(&args->log.all_hex_limit)->default_value(32), "ALL log hex dump limit")

    ("log.filename", bpo::value<string>(&args->log.filename)->default_value("/tmp/ue.log"), "Log filename")
    ("log.file_max_size", bpo::value<int>(&args->log.file_max_size)->default_value(-1), "Maximum file size (in kilobytes). When passed, multiple files are created. Default -1 (single file)")

    ("usim.mode", bpo::value<string>(&args->stack.usim.mode)->default_value("soft"), "USIM mode (soft or pcsc)")
    ("usim.algo", bpo::value<string>(&args->stack.usim.algo), "USIM authentication algorithm")
    ("usim.op", bpo::value<string>(&args->stack.usim.op), "USIM operator code")
    ("usim.opc", bpo::value<string>(&args->stack.usim.opc), "USIM operator code (ciphered variant)")
    ("usim.imsi", bpo::value<string>(&args->stack.usim.imsi), "USIM IMSI")
    ("usim.imei", bpo::value<string>(&args->stack.usim.imei), "USIM IMEI")
    ("usim.k", bpo::value<string>(&args->stack.usim.k), "USIM K")
    ("usim.pin", bpo::value<string>(&args->stack.usim.pin), "PIN in case real SIM card is used")
    ("usim.reader", bpo::value<string>(&args->stack.usim.reader)->default_value(""), "Force specific PCSC reader. Default: Try all available readers.")

    ("gw.netns", bpo::value<string>(&args->gw.netns)->default_value(""), "Network namespace to for TUN device (empty for default netns)")
    ("gw.ip_devname", bpo::value<string>(&args->gw.tun_dev_name)->default_value("tun_srsue"), "Name of the tun_srsue device")
    ("gw.ip_netmask", bpo::value<string>(&args->gw.tun_dev_netmask)->default_value("255.255.255.0"), "Netmask of the tun_srsue device")

    /* Downlink Channel emulator section */
    ("channel.dl.enable",            bpo::value<bool>(&args->phy.dl_channel_args.enable)->default_value(false),                 "Enable/Disable internal Downlink channel emulator")
    ("channel.dl.awgn.enable",       bpo::value<bool>(&args->phy.dl_channel_args.awgn_enable)->default_value(false),            "Enable/Disable AWGN simulator")
    ("channel.dl.awgn.snr",          bpo::value<float>(&args->phy.dl_channel_args.awgn_snr_dB)->default_value(30.0f),           "SNR in dB")
    ("channel.dl.awgn.signal_power", bpo::value<float>(&args->phy.dl_channel_args.awgn_signal_power_dBfs)->default_value(0.0f), "Received signal power in decibels full scale (dBfs)")
    ("channel.dl.fading.enable",     bpo::value<bool>(&args->phy.dl_channel_args.fading_enable)->default_value(false),          "Enable/Disable Fading model")
    ("channel.dl.fading.model",      bpo::value<string>(&args->phy.dl_channel_args.fading_model)->default_value("none"),   "Fading model + maximum doppler (E.g. none, epa5, eva70, etu300, etc)")
    ("channel.dl.delay.enable",      bpo::value<bool>(&args->phy.dl_channel_args.delay_enable)->default_value(false),           "Enable/Disable Delay simulator")
    ("channel.dl.delay.period_s",    bpo::value<float>(&args->phy.dl_channel_args.delay_period_s)->default_value(3600),         "Delay period in seconds (integer)")
    ("channel.dl.delay.init_time_s", bpo::value<float>(&args->phy.dl_channel_args.delay_init_time_s)->default_value(0),         "Initial time in seconds")
    ("channel.dl.delay.maximum_us",  bpo::value<float>(&args->phy.dl_channel_args.delay_max_us)->default_value(100.0f),         "Maximum delay in microseconds")
    ("channel.dl.delay.minimum_us",  bpo::value<float>(&args->phy.dl_channel_args.delay_min_us)->default_value(10.0f),          "Minimum delay in microseconds")
    ("channel.dl.rlf.enable",        bpo::value<bool>(&args->phy.dl_channel_args.rlf_enable)->default_value(false),             "Enable/Disable Radio-Link Failure simulator")
    ("channel.dl.rlf.t_on_ms",       bpo::value<uint32_t >(&args->phy.dl_channel_args.rlf_t_on_ms)->default_value(10000),       "Time for On state of the channel (ms)")
    ("channel.dl.rlf.t_off_ms",      bpo::value<uint32_t >(&args->phy.dl_channel_args.rlf_t_off_ms)->default_value(2000),       "Time for Off state of the channel (ms)")
    ("channel.dl.hst.enable",        bpo::value<bool>(&args->phy.dl_channel_args.hst_enable)->default_value(false),             "Enable/Disable HST simulator")
    ("channel.dl.hst.period_s",      bpo::value<float>(&args->phy.dl_channel_args.hst_period_s)->default_value(7.2f),           "HST simulation period in seconds")
    ("channel.dl.hst.fd_hz",         bpo::value<float>(&args->phy.dl_channel_args.hst_fd_hz)->default_value(+750.0f),           "Doppler frequency in Hz")
    ("channel.dl.hst.init_time_s",   bpo::value<float>(&args->phy.dl_channel_args.hst_init_time_s)->default_value(0),           "Initial time in seconds")

    /* Uplink Channel emulator section */
    ("channel.ul.enable",            bpo::value<bool>(&args->phy.ul_channel_args.enable)->default_value(false),                  "Enable/Disable internal Downlink channel emulator")
    ("channel.ul.awgn.enable",       bpo::value<bool>(&args->phy.ul_channel_args.awgn_enable)->default_value(false),             "Enable/Disable AWGN simulator")
    ("channel.ul.awgn.snr",          bpo::value<float>(&args->phy.ul_channel_args.awgn_snr_dB)->default_value(30.0f),            "Noise level in decibels full scale (dBfs)")
    ("channel.ul.awgn.signal_power", bpo::value<float>(&args->phy.ul_channel_args.awgn_signal_power_dBfs)->default_value(30.0f), "Transmitted signal power in decibels full scale (dBfs)")
    ("channel.ul.fading.enable",     bpo::value<bool>(&args->phy.ul_channel_args.fading_enable)->default_value(false),           "Enable/Disable Fading model")
    ("channel.ul.fading.model",      bpo::value<string>(&args->phy.ul_channel_args.fading_model)->default_value("none"),    "Fading model + maximum doppler (E.g. none, epa5, eva70, etu300, etc)")
    ("channel.ul.delay.enable",      bpo::value<bool>(&args->phy.ul_channel_args.delay_enable)->default_value(false),            "Enable/Disable Delay simulator")
    ("channel.ul.delay.period_s",    bpo::value<float>(&args->phy.ul_channel_args.delay_period_s)->default_value(3600),          "Delay period in seconds (integer)")
    ("channel.ul.delay.init_time_s", bpo::value<float>(&args->phy.ul_channel_args.delay_init_time_s)->default_value(0),          "Initial time in seconds")
    ("channel.ul.delay.maximum_us",  bpo::value<float>(&args->phy.ul_channel_args.delay_max_us)->default_value(100.0f),          "Maximum delay in microseconds")
    ("channel.ul.delay.minimum_us",  bpo::value<float>(&args->phy.ul_channel_args.delay_min_us)->default_value(10.0f),           "Minimum delay in microseconds")
    ("channel.ul.rlf.enable",        bpo::value<bool>(&args->phy.ul_channel_args.rlf_enable)->default_value(false),              "Enable/Disable Radio-Link Failure simulator")
    ("channel.ul.rlf.t_on_ms",       bpo::value<uint32_t >(&args->phy.ul_channel_args.rlf_t_on_ms)->default_value(10000),        "Time for On state of the channel (ms)")
    ("channel.ul.rlf.t_off_ms",      bpo::value<uint32_t >(&args->phy.ul_channel_args.rlf_t_off_ms)->default_value(2000),        "Time for Off state of the channel (ms)")
    ("channel.ul.hst.enable",        bpo::value<bool>(&args->phy.ul_channel_args.hst_enable)->default_value(false),              "Enable/Disable HST simulator")
    ("channel.ul.hst.period_s",      bpo::value<float>(&args->phy.ul_channel_args.hst_period_s)->default_value(7.2f),            "HST simulation period in seconds")
    ("channel.ul.hst.fd_hz",         bpo::value<float>(&args->phy.ul_channel_args.hst_fd_hz)->default_value(+750.0f),            "Doppler frequency in Hz")
    ("channel.ul.hst.init_time_s",   bpo::value<float>(&args->phy.ul_channel_args.hst_init_time_s)->default_value(0),            "Initial time in seconds")

    /* CFR section */
    ("cfr.enable", bpo::value<bool>(&args->phy.cfr_args.enable)->default_value(args->phy.cfr_args.enable), "CFR enable")
    ("cfr.mode", bpo::value<string>(&cfr_mode)->default_value("manual"), "CFR mode")
    ("cfr.manual_thres", bpo::value<float>(&args->phy.cfr_args.manual_thres)->default_value(args->phy.cfr_args.manual_thres), "Fixed manual clipping threshold for CFR manual mode")
    ("cfr.strength", bpo::value<float>(&args->phy.cfr_args.strength)->default_value(args->phy.cfr_args.strength), "CFR ratio between amplitude-limited vs original signal (0 to 1)")
    ("cfr.auto_target_papr", bpo::value<float>(&args->phy.cfr_args.auto_target_papr)->default_value(args->phy.cfr_args.auto_target_papr), "Signal PAPR target (in dB) in CFR auto modes")
    ("cfr.ema_alpha", bpo::value<float>(&args->phy.cfr_args.ema_alpha)->default_value(args->phy.cfr_args.ema_alpha), "Alpha coefficient for the power average in auto_ema mode (0 to 1)")

    /* PHY section */
    ("phy.worker_cpu_mask",
     bpo::value<int>(&args->phy.worker_cpu_mask)->default_value(-1),
     "cpu bit mask (eg 255 = 1111 1111)")

    ("phy.sync_cpu_affinity",
     bpo::value<int>(&args->phy.sync_cpu_affinity)->default_value(-1),
     "index of the core used by the sync thread")

    ("phy.rx_gain_offset",
     bpo::value<float>(&args->phy.rx_gain_offset)->default_value(62),
     "RX Gain offset to add to rx_gain to correct RSRP value")

    ("phy.prach_gain",
     bpo::value<float>(&args->phy.prach_gain)->default_value(-1.0),
     "Disable PRACH power control")

    ("phy.cqi_max",
     bpo::value<int>(&args->phy.cqi_max)->default_value(15),
     "Upper bound on the maximum CQI to be reported. Default 15.")

    ("phy.cqi_fixed",
     bpo::value<int>(&args->phy.cqi_fixed)->default_value(-1),
     "Fixes the reported CQI to a constant value. Default disabled.")

    ("phy.sfo_correct_period",
     bpo::value<uint32_t>(&args->phy.sfo_correct_period)->default_value(DEFAULT_SAMPLE_OFFSET_CORRECT_PERIOD),
     "Period in ms to correct sample time")

    ("phy.sfo_emma",
     bpo::value<float>(&args->phy.sfo_ema)->default_value(DEFAULT_SFO_EMA_COEFF),
     "EMA coefficient to average sample offsets used to compute SFO")

    ("phy.snr_ema_coeff",
     bpo::value<float>(&args->phy.snr_ema_coeff)->default_value(0.1),
     "Sets the SNR exponential moving average coefficient (Default 0.1)")

    ("phy.snr_estim_alg",
     bpo::value<string>(&args->phy.snr_estim_alg)->default_value("refs"),
     "Sets the noise estimation algorithm. (Default refs)")

    ("phy.pdsch_max_its",
     bpo::value<uint32_t>(&args->phy.pdsch_max_its)->default_value(8),
     "Maximum number of turbo decoder iterations")

    ("phy.meas_evm",
     bpo::value<bool>(&args->phy.meas_evm)->default_value(false),
     "Measure PDSCH EVM, increases CPU load (default false)")

    ("phy.nof_phy_threads",
     bpo::value<uint32_t>(&args->phy.nof_phy_threads)->default_value(3),
     "Number of PHY threads")

    ("phy.equalizer_mode",
     bpo::value<string>(&args->phy.equalizer_mode)->default_value("mmse"),
     "Equalizer mode")

    ("phy.intra_freq_meas_len_ms",
       bpo::value<uint32_t>(&args->phy.intra_freq_meas_len_ms)->default_value(20),
       "Duration of the intra-frequency neighbour cell measurement in ms.")

    ("phy.intra_freq_meas_period_ms",
       bpo::value<uint32_t>(&args->phy.intra_freq_meas_period_ms)->default_value(200),
       "Period of intra-frequency neighbour cell measurement in ms. Maximum as per 3GPP is 200 ms.")

    ("phy.correct_sync_error",
       bpo::value<bool>(&args->phy.correct_sync_error)->default_value(false),
       "Channel estimator measures and pre-compensates time synchronization error. Increases CPU usage, improves PDSCH "
       "decoding in high SFO and high speed UE scenarios.")

    ("phy.cfo_is_doppler",
       bpo::value<bool>(&args->phy.cfo_is_doppler)->default_value(false),
       "Assume detected CFO is doppler and correct the UL in the same direction. If disabled, the CFO is assumed"
       "to be caused by the local oscillator and the UL correction is in the opposite direction. Default assumes oscillator.")

    ("phy.cfo_integer_enabled",
     bpo::value<bool>(&args->phy.cfo_integer_enabled)->default_value(false),
     "Enables integer CFO estimation and correction.")

    ("phy.cfo_correct_tol_hz",
     bpo::value<float>(&args->phy.cfo_correct_tol_hz)->default_value(1.0),
     "Tolerance (in Hz) for digital CFO compensation (needs to be low if interpolate_subframe_enabled=true.")

    ("phy.cfo_pss_ema",
     bpo::value<float>(&args->phy.cfo_pss_ema)->default_value(DEFAULT_CFO_EMA_TRACK),
     "CFO Exponential Moving Average coefficient for PSS estimation during TRACK.")

    ("phy.cfo_ref_mask",
     bpo::value<uint32_t>(&args->phy.cfo_ref_mask)->default_value(1023),
     "Bitmask for subframes on which to run RS estimation (set to 0 to disable, default all sf)")

    ("phy.cfo_loop_bw_pss",
     bpo::value<float>(&args->phy.cfo_loop_bw_pss)->default_value(DEFAULT_CFO_BW_PSS),
     "CFO feedback loop bandwidth for samples from PSS")

    ("phy.cfo_loop_bw_ref",
     bpo::value<float>(&args->phy.cfo_loop_bw_ref)->default_value(DEFAULT_CFO_BW_REF),
     "CFO feedback loop bandwidth for samples from RS")

    ("phy.cfo_loop_pss_tol",
     bpo::value<float>(&args->phy.cfo_loop_pss_tol)->default_value(DEFAULT_CFO_PSS_MIN),
     "Tolerance (in Hz) of the PSS estimation method. Below this value, PSS estimation does not feeds back the loop"
     "and RS estimations are used instead (when available)")

    ("phy.cfo_loop_ref_min",
     bpo::value<float>(&args->phy.cfo_loop_ref_min)->default_value(DEFAULT_CFO_REF_MIN),
     "Tolerance (in Hz) of the RS estimation method. Below this value, RS estimation does not feeds back the loop")

    ("phy.cfo_loop_pss_conv",
     bpo::value<uint32_t>(&args->phy.cfo_loop_pss_conv)->default_value(DEFAULT_PSS_STABLE_TIMEOUT),
     "After the PSS estimation is below cfo_loop_pss_tol for cfo_loop_pss_timeout times consecutively, RS adjustments are allowed.")

    ("phy.interpolate_subframe_enabled",
     bpo::value<bool>(&args->phy.interpolate_subframe_enabled)->default_value(false),
     "Interpolates in the time domain the channel estimates within 1 subframe.")

    ("phy.estimator_fil_auto",
     bpo::value<bool>(&args->phy.estimator_fil_auto)->default_value(false),
     "The channel estimator smooths the channel estimate with an adaptative filter.")

    ("phy.estimator_fil_stddev",
     bpo::value<float>(&args->phy.estimator_fil_stddev)->default_value(1.0f),
     "Sets the channel estimator smooth gaussian filter standard deviation.")

    ("phy.estimator_fil_order",
     bpo::value<uint32_t>(&args->phy.estimator_fil_order)->default_value(4),
     "Sets the channel estimator smooth gaussian filter order (even values perform better).")

    ("phy.snr_to_cqi_offset",
     bpo::value<float>(&args->phy.snr_to_cqi_offset)->default_value(0),
     "Sets an offset in the SNR to CQI table. This is used to adjust the reported CQI.")

    ("phy.sss_algorithm",
     bpo::value<string>(&args->phy.sss_algorithm)->default_value("full"),
     "Selects the SSS estimation algorithm.")

    ("phy.pdsch_csi_enabled",
     bpo::value<bool>(&args->phy.pdsch_csi_enabled)->default_value(true),
     "Stores the Channel State Information and uses it for weightening the softbits. It is only used in TM1.")

    ("phy.pdsch_8bit_decoder",
       bpo::value<bool>(&args->phy.pdsch_8bit_decoder)->default_value(false),
       "Use 8-bit for LLR representation and turbo decoder trellis computation (Experimental)")

    ("phy.force_ul_amplitude",
       bpo::value<float>(&args->phy.force_ul_amplitude)->default_value(0.0),
       "Forces the peak amplitude in the PUCCH, PUSCH and SRS (set 0.0 to 1.0, set to 0 or negative for disabling)")

    ("phy.detect_cp",
      bpo::value<bool>(&args->phy.detect_cp)->default_value(false),
      "enable CP length detection")

    ("phy.in_sync_rsrp_dbm_th",
     bpo::value<float>(&args->phy.in_sync_rsrp_dbm_th)->default_value(-130.0f),
     "RSRP threshold (in dBm) above which the UE considers to be in-sync")

    ("phy.in_sync_snr_db_th",
     bpo::value<float>(&args->phy.in_sync_snr_db_th)->default_value(3.0f),
     "SNR threshold (in dB) above which the UE considers to be in-sync")

    ("phy.nof_in_sync_events",
     bpo::value<uint32_t>(&args->phy.nof_in_sync_events)->default_value(10),
     "Number of PHY in-sync events before sending an in-sync event to RRC")

    ("phy.nof_out_of_sync_events",
     bpo::value<uint32_t>(&args->phy.nof_out_of_sync_events)->default_value(20),
     "Number of PHY out-sync events before sending an out-sync event to RRC")

    ("expert.lte_sample_rates",
     bpo::value<bool>(&use_standard_lte_rates)->default_value(false),
     "Whether to use default LTE sample rates instead of shorter variants.")

    ("phy.force_N_id_2",
     bpo::value<int>(&args->phy.force_N_id_2)->default_value(-1),
     "Force using a specific PSS (set to -1 to allow all PSSs).")

    ("phy.force_N_id_1",
     bpo::value<int>(&args->phy.force_N_id_1)->default_value(-1),
     "Force using a specific SSS (set to -1 to allow all SSSs).")

    // PHY NR args
    ("phy.nr.store_pdsch_ko",
      bpo::value<bool>(&args->phy.nr_store_pdsch_ko)->default_value(false),
      "Dumps the PDSCH baseband samples into a file on KO reception.")

    // UE simulation args
    ("sim.airplane_t_on_ms",
     bpo::value<int>(&args->stack.nas.sim.airplane_t_on_ms)->default_value(-1),
     "On-time for airplane mode (in ms)")

    ("sim.airplane_t_off_ms",
     bpo::value<int>(&args->stack.nas.sim.airplane_t_off_ms)->default_value(-1),
     "Off-time for airplane mode (in ms)")

     /* general options */
    ("general.metrics_period_secs",
       bpo::value<float>(&args->general.metrics_period_secs)->default_value(1.0),
      "Periodicity for metrics in seconds")

    ("general.metrics_csv_enable",
       bpo::value<bool>(&args->general.metrics_csv_enable)->default_value(false),
       "Write UE metrics to CSV file")

    ("general.metrics_csv_filename",
       bpo::value<string>(&args->general.metrics_csv_filename)->default_value("/tmp/ue_metrics.csv"),
       "Metrics CSV filename")

    ("general.metrics_csv_append",
           bpo::value<bool>(&args->general.metrics_csv_append)->default_value(false),
           "Set to true to append new output to existing CSV file")

    ("general.metrics_csv_flush_period_sec",
           bpo::value<int>(&args->general.metrics_csv_flush_period_sec)->default_value(-1),
           "Periodicity in s to flush CSV file to disk (-1 for auto)")

    ("general.metrics_json_enable",
     bpo::value<bool>(&args->general.metrics_json_enable)->default_value(false),
     "Write UE metrics to a JSON file")

    ("general.metrics_json_filename",
     bpo::value<string>(&args->general.metrics_json_filename)->default_value("/tmp/ue_metrics.json"),
     "Metrics JSON filename")

    ("general.tracing_enable",
           bpo::value<bool>(&args->general.tracing_enable)->default_value(false),
           "Events tracing")

    ("general.tracing_filename",
           bpo::value<string>(&args->general.tracing_filename)->default_value("/tmp/ue_tracing.log"),
           "Tracing events filename")

    ("general.tracing_buffcapacity",
           bpo::value<size_t>(&args->general.tracing_buffcapacity)->default_value(1000000),
           "Tracing buffer capcity")

    ("stack.have_tti_time_stats",
        bpo::value<bool>(&args->stack.have_tti_time_stats)->default_value(true),
        "Calculate TTI execution statistics")

    ;

    ifstream conf(config_file.c_str(), ios::in);
    if (!conf) {
        cerr << "\nCannot open config file: " << config_file << endl;
        return 1;
    }

    bpo::variables_map vm;

    try {
        bpo::store(bpo::parse_config_file(conf, common), vm);
        bpo::notify(vm);
    } catch (const bpo::error& e) {
        cerr << "\nError parsing config file: " << e.what() << endl;
        return 2;
    }

    return 0;

}


class zmq_ue{
    public:

        zmq_ue(pair<string,string> ports, all_args_t args){
        this->ports = ports;
        this->args = args;
        }   

        pair<string,string> ports;
        all_args_t args;

        bool operator==(const zmq_ue& other) const {
        return (this->ports == other.ports);
    }
};


void my_handler(int s){
    printf("\n===== End ZMQ =====\n");
    
    exit(1); 
}

pair<string, string> extract_port(string device_args){
    regex tx_regex("tx_port=(\\d{4})");
    regex rx_regex("rx_port=(\\d{4})");

    string tx_port, rx_port;

    smatch match;

    if (regex_search(device_args, match, tx_regex) && match.size() > 1) {
        tx_port = (match[1].str());
    }

    if (regex_search(device_args, match, rx_regex) && match.size() > 1) {
        rx_port = match[1].str();
    }

    return {tx_port, rx_port};
}

std::vector<std::complex<float>> byte_to_complex(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(std::complex<float>) + 1) {
        std::cerr << "Error: buffer too small to contain any complex samples\n";
        return {};
    }

    size_t num_samples = (buffer.size()) / sizeof(std::complex<float>);

    if (num_samples > std::numeric_limits<size_t>::max() / sizeof(std::complex<float>)) {
        std::cerr << "Error: too many samples, would overflow\n";
        return {};
    }

    std::vector<std::complex<float>> result(num_samples);
    memcpy(result.data(), buffer.data() + 1, num_samples * sizeof(std::complex<float>));
    
    return result;
}


int send_to_matlab(void *socket_to_matlab, std::vector<uint8_t> &data, int data_size, char *id_data,size_t id_size){

    zmq_msg_t recv_id;
    zmq_msg_t recv_data;
    size_t recv_size;
    zmq_msg_t new_id_msg;

    int size = 0;

    zmq_msg_init_size(&new_id_msg, id_size);
    memcpy(zmq_msg_data(&new_id_msg), id_data, id_size);

    //send ID
    zmq_msg_send(&new_id_msg, socket_to_matlab, ZMQ_SNDMORE);

    zmq_msg_close(&new_id_msg);

    size = zmq_send(socket_to_matlab, data.data(), data_size, 0);

    return size;
}

int receive_from_matlab(void *socket_to_matlab, std::vector<uint8_t> &data, int target_rcv_data) {
    zmq_msg_t recv_id, recv_data;
    int sum_rcv_data = 0;
    int rc;

    data.clear();

    while (sum_rcv_data < target_rcv_data) {
        zmq_msg_init(&recv_id);
        rc = zmq_msg_recv(&recv_id, socket_to_matlab, 0);
        if (rc <= 0) {
            zmq_msg_close(&recv_id);
            fprintf(stderr, "\nFailed to receive ID\n");
            continue;
        }
        zmq_msg_close(&recv_id); 

        zmq_msg_init(&recv_data);
        rc = zmq_msg_recv(&recv_data, socket_to_matlab, 0);
        if (rc <= 0) {
            zmq_msg_close(&recv_data);
            fprintf(stderr, "\nFailed to receive packet\n");
            break;
        }

        size_t recv_size = zmq_msg_size(&recv_data);
        uint8_t* recv_ptr = static_cast<uint8_t*>(zmq_msg_data(&recv_data));

        int bytes_to_copy = std::min(static_cast<int>(recv_size), target_rcv_data - sum_rcv_data);

        data.insert(data.end(), recv_ptr, recv_ptr + bytes_to_copy);
        sum_rcv_data += bytes_to_copy;

        printf("socket_to_matlab [receive data] = %d (total = %d)\n", bytes_to_copy, sum_rcv_data);

        if (bytes_to_copy < static_cast<int>(recv_size)) {
            size_t extra = recv_size - bytes_to_copy;
            printf("\nExtra data (%zu bytes) ignored\n", extra);
            zmq_msg_close(&recv_data);
            break;
        }

        zmq_msg_close(&recv_data);
    }

    return sum_rcv_data;
}

template <typename T>
vector<uint8_t> to_byte(vector<T> data){

    int byte_size = data.size() * sizeof(T);

    vector<uint8_t> result(byte_size);

    memcpy(result.data(), data.data(), byte_size);

    return result;
}

vector<uint8_t> concatenate_all_samples(vector<vector<complex<float>>> &all_samples){

    int len = 0; //len samples in bytes

    for(const auto &samples: all_samples){
        len += 1; // ID
        len += samples.size() * sizeof(complex<float>);
    }

    vector<uint8_t> result(len);
    int offset = 0;

    for(int i = 0; i < all_samples.size(); ++i){

        result[offset] = static_cast<uint8_t>(i);

        offset++;

        memcpy(result.data() + offset, all_samples[i].data(), all_samples[i].size() * sizeof(complex<float>));
        offset += all_samples[i].size() * sizeof(complex<float>);
    }

    return result;
}


vector<vector<complex<float>>> deconcatenate_all_samples(vector<uint8_t> &all_samples, vector<int> packet_sizes) {

    vector<vector<complex<float>>> result(packet_sizes.size());
    int offset = 0;

    for(int i = 0; i < packet_sizes.size(); ++i){
        result[i].resize(packet_sizes[i]);
        memcpy(result[i].data(), all_samples.data() + offset + 1, packet_sizes[i]);

        offset += packet_sizes[i];
    }

    return result;

}




int main(int argc, char *argv[]){

    if(argc == 1){
        cout << "\nConfig file not found\n";
        return 0;
    }

    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
 
    sigaction(SIGINT, &sigIntHandler, NULL);

    pair<string,string> ports;

    vector<zmq_ue> ues; //all ue

    for(int config_num = 1; config_num < argc; ++config_num){

        all_args_t args;

        string config_file = argv[config_num];

        cout << "\n===== Start parsing " << config_file << " =====\n";

        if(ue_parser(&args, config_file) != 0){
            cout << "\n\n===== End parsing " << config_file << " =====\n";
            continue;
        }

        ports = extract_port(args.rf.device_args);

        zmq_ue ue(ports, args);

        if (find(ues.begin(), ues.end(), ue) != ues.end()) {
            cout << "\nThis ue already exist\n";
            cout << "\n===== End parsing " << config_file << " =====\n";
            continue;
        }

        ues.push_back(ue);

        cout << "\ntx_port=" << ports.first << "\nrx_ports=" << ports.second << "\n\n===== End parsing " << config_file << " =====\n";
    }

    int num_ues = ues.size();

    if(num_ues == 0){
        cout << "\nUe not found\n";
        exit(1);
    }

    cout << "\n========= Start ZMQ =========\n";

    cout << "ZMQ_MAX_BUFFER_SIZE = " << ZMQ_MAX_BUFFER_SIZE << endl;
    cout << "NBYTES_PER_ONE_SAMPLE = " << NBYTES_PER_ONE_SAMPLE << endl;
    cout << "sizeof(cf_t) = " << sizeof(cf_t) << endl;

    int num_gnbs = 1;

    int port_gnb_tx = 2000;
    int port_gnb_rx = 2001;

    string addr_recv_port_gnb_tx = "tcp://localhost:" + to_string(port_gnb_tx);
    string addr_send_port_gnb_rx = "tcp://*:" + to_string(port_gnb_rx);

    //Address
    vector<string> addr_recv_ports_ue_tx(num_ues);
    vector<string> addr_send_ports_ue_rx(num_ues);


    for(int i = 0; i < num_ues; i++){
        addr_send_ports_ue_rx[i] = "tcp://*:" + ues[i].ports.second;
        addr_recv_ports_ue_tx[i] = "tcp://localhost:" + ues[i].ports.first;
    }

    // Initialize ZMQ
    void *context = zmq_ctx_new ();

    void *req_socket_from_gnb_tx = zmq_socket(context, ZMQ_REQ);

    vector<void*>req_sockets_from_ue_tx(num_ues);

    for(int i = 0; i < num_ues; ++i){
        req_sockets_from_ue_tx[i] = zmq_socket(context, ZMQ_REQ);
    }

    if(zmq_connect(req_socket_from_gnb_tx, addr_recv_port_gnb_tx.c_str())){
        perror("zmq_connect\n");
        return 1;
    } else {
        printf("zmq_connect [req_socket_for_gnb_tx] - success\n");
    }

    for(int i = 0; i < num_ues; ++i){
        if(zmq_connect(req_sockets_from_ue_tx[i], addr_recv_ports_ue_tx[i].c_str())){
            perror("zmq_connect\n");
            return 1;
        } else {
            printf("zmq_connect [req_socket_for_UE%d_tx] - success\n", i+1);
        }   
    }

    void *send_socket_for_gnb_rx = zmq_socket(context, ZMQ_REP);

    vector<void*>send_sockets_for_ue_rx(num_ues);

    for(int i = 0; i < num_ues; ++i){
        send_sockets_for_ue_rx[i] = zmq_socket(context, ZMQ_REP);
    }

    if(zmq_bind(send_socket_for_gnb_rx, addr_send_port_gnb_rx.c_str())) {
        perror("zmq_bind\n");
        return 1;
    } else {
        printf("zmq_bind [send_socket_for_gnb_rx]- success\n");
    }

    for(int i = 0; i < num_ues; ++i){
        if(zmq_bind(send_sockets_for_ue_rx[i], addr_send_ports_ue_rx[i].c_str())){
            perror("zmq_connect\n");
            return 1;
        } else {
            printf("zmq_connect [req_socket_for_UE%d_rx] - success\n", i+1);
        }   
    }

    // //socket to matlab
    void *socket_to_matlab = zmq_socket (context, ZMQ_STREAM);

    if(zmq_bind(socket_to_matlab, "tcp://*:4000")){
        perror("zmq_bind");
        return 1;
    } else {
        printf("zmq_bind success\n");
    }

    int timeout = 25000;

    zmq_setsockopt(req_socket_from_gnb_tx, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

    for(int i = 0; i < num_ues; ++i){
        zmq_setsockopt(req_sockets_from_ue_tx[i], ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    }

    using cf_t = std::complex<float>;

    zmq_msg_t id_msg;

    size_t id_size;
    char *id_data;

    int N = 80000;

    vector<cf_t> buffer_vec(N);
    vector<cf_t> concatenate_buffer(N);

    char buffer_acc[10];
    int broker_rcv_accept_ues[num_ues] = {0};
    int broker_rcv_accept_gnbs[num_gnbs] = {0};
    int size;
    int send;
    int gnb_data_size = 0;
    int send_to_matlab_size = 0;
    uint8_t gnb_num = 0;

    vector<int> device_accepts_tx;
    device_accepts_tx.resize(num_gnbs + num_ues);

    int nbytes = N * sizeof(cf_t);

    vector<vector<cf_t>> all_samples(num_gnbs + num_ues);
    vector<uint8_t> concatenate_samples;

    vector<int> samples_size(num_gnbs + num_ues);
    vector<uint8_t> byte_sample_size;

    bool matlab_connected = false;

    vector<uint8_t> matlab_send_ack(num_ues + num_gnbs);

    int matlab_send_acc_sum = 0; 

    vector<uint8_t> matlab_buffer(buffer_vec.size() * sizeof(cf_t) + 1);

    vector<cf_t> samples;


    while(1){

        //check matlab conection

        if(!matlab_connected){

            zmq_msg_init(&id_msg);
            zmq_msg_recv(&id_msg, socket_to_matlab, 0);
    
            id_size = zmq_msg_size(&id_msg);
            id_data = new char[id_size];
    
            memcpy(id_data, zmq_msg_data(&id_msg), id_size);
    
            if(id_size > 0){
                printf("\nMatlab connected successfully\n");
                matlab_connected = true;
            } 
            
            else{
                continue;
            }
    
            zmq_msg_close(&id_msg);
        }

        //receive conn accept from RX's

        device_accepts_tx.resize(num_gnbs + num_ues, 0);

        memset(buffer_acc, 0, sizeof(buffer_acc));

        size = zmq_recv(send_socket_for_gnb_rx, buffer_acc, sizeof(buffer_acc), 0);

        if(size == -1){

            printf("!!!!!!!!!!!   -----1 send_socket_for_gnb_rx\n");

            printf(zmq_strerror(zmq_errno()));

            continue;

        } else{

            broker_rcv_accept_gnbs[0] = 1;

            printf("broker received [buffer_acc] form GNB RX = %d\n", size);
        }

        for(int i = 0; i < num_ues; ++i){

            memset(buffer_acc, 0, sizeof(buffer_acc));

            size = zmq_recv(send_sockets_for_ue_rx[i], buffer_acc, sizeof(buffer_acc), 0);

            if(size == -1){

                printf("!!!!!!!!!!!   -----1 send_socket_for_ue_%d_rx\n", i+1);

                continue;

            } else{
                broker_rcv_accept_ues[i] = 1;

                printf("broker received [buffer_acc] from UE[%d] RX = %d\n", i+1, size);
            }
        }
    

        int summ = 0;

        for (int i = 0; i < num_ues; i++){
            summ += broker_rcv_accept_ues[i];
        }

        printf("Accept UES: %d\n", summ);

        if (summ == num_ues && broker_rcv_accept_gnbs[0] == num_gnbs)
        {
            int send = 0;

            // send accepts to TX's
            send = zmq_send(req_socket_from_gnb_tx, buffer_acc, size, 0);

            printf("req_socket_from_gnb_tx [send] = %d\n", send);

            for(int i = 0; i < num_ues; ++i){
                send = zmq_send(req_sockets_from_ue_tx[i], buffer_acc, size, 0);
                printf("req_socket_from_ue_%d_tx [send] = %d\n", i+1 ,send);
            }


            // start data transmissiona

            fill(buffer_vec.begin(), buffer_vec.end(), 0);

            //recv from gnb
            size = zmq_recv(req_socket_from_gnb_tx, (void*)buffer_vec.data(), nbytes, 0);

            if (size == -1){
                printf("ERROR: broker received from gNb = %d\n", size, buffer_vec.size());
                continue;
            }

            samples_size[0] = size;

            printf("broker received from gNb =  %d\n", size);

            all_samples[0].clear();

            all_samples[0].resize(size / 8);

            memcpy(all_samples[0].data(), buffer_vec.data(), size);

            printf("\nrecieved from gnb\n");
            
            //check first 100 samples
            for(int j = 0; j < 100; ++j){
                cout << buffer_vec[j] << ", ";
            }

            printf("\n");

            //recieve samples from ues
            for(int i = 0; i < num_ues; ++i){

                fill(buffer_vec.begin(), buffer_vec.end(), 0);

                size = zmq_recv(req_sockets_from_ue_tx[i], (void*)buffer_vec.data(), nbytes, 0);

                if (size == -1){
                   continue;
                }

                samples_size[i + 1] = size;


                all_samples[i + 1].resize(size / 8);

                memcpy(all_samples[i + 1].data(), buffer_vec.data(), size);

                printf("\nbroker [received data] from UE[%d] %d size packet\n", i+1 ,size);
            }

            //send to MATLAB packet sizes

            byte_sample_size = to_byte(samples_size);

            size = send_to_matlab(socket_to_matlab, byte_sample_size, byte_sample_size.size(), id_data, id_size);

            printf("\nsocket to matlab [send data] (packet size) = %d\n", size);

            //concatenate ues and gnb samples


            concatenate_samples = concatenate_all_samples(all_samples);

            printf("concatenate samples size = %d", concatenate_samples.size());

            // //send all samples to MATLB

            size = send_to_matlab(socket_to_matlab, concatenate_samples, concatenate_samples.size(), id_data, id_size);

            printf("\nsocket to matlab [send data] = %d\n", size);

            // //receive all samples from matlab

            matlab_buffer.resize(size);

            size = receive_from_matlab(socket_to_matlab, matlab_buffer, size);

            printf("\nbroker [received data] from MATLAB = %d\n", size);

            // //deconcatenate samples from MATLAB

            vector<vector<cf_t>> deconcatenate_samples = deconcatenate_all_samples(matlab_buffer, samples_size);

            // //send data to ues

            for(int i = 0; i < num_ues; ++i){
                send = zmq_send(send_sockets_for_ue_rx[i], all_samples[0].data(), samples_size[0], 0);
                printf("send_socket_for_ue_%d_rx [send data] = %d\n", i + 1 ,send);
            }

            // //concatenate ues sameples

            int max_size = -1;

            for(int i = 1; i < samples_size.size(); ++i){
                if(samples_size[i] > max_size){
                    max_size = samples_size[i];
                }
            }

            printf("max ues packet size: %d", max_size);

            concatenate_buffer.clear();

            concatenate_buffer.resize(max_size / 8);

            for(int i = 1; i < all_samples.size(); ++i){
                transform(concatenate_buffer.begin(), concatenate_buffer.end(), all_samples[i].begin(), concatenate_buffer.begin(), std::plus<std::complex<float>>());
            }

            // //send data to gnb

            send = zmq_send(send_socket_for_gnb_rx, (void*)concatenate_buffer.data(), max_size, 0);

            printf("\nsend_socket_for_gnb_rx [send data] = %d\n", send);
            
        }
           
        else
        {   
            continue;
        }

        usleep(100);
    }


    return 0;
}  
#!/usr/bin/env bash

pids=()
CFG_PATH="$PWD/configs"
# USER_HOME="$HOME" # or... 
# USER_HOME="/home/<YOUR USER>" 
CURRENT_DIR=$(pwd)
LAUNCH_USER=$(whoami)
USER_HOME="/home/$(whoami)" 

NUM_UES=2

echo $CFG_PATH
echo $CURRENT_DIR
start_program() {
    sudo xterm  -e "$1" -c "$2" &
    pids+=($!)
    sleep 0.2
}


# Start gNb
start_program "$CURRENT_DIR/srsRAN_Project/build/apps/gnb/gnb" "$CFG_PATH/gnb_zmq.yaml"
sleep 0.2

# sudo xterm -hold -e "$CURRENT_DIR/srsRAN_4G/build/srsue/src/srsue" "$CFG_PATH/ue11_zmq.conf"
# sleep 0.2
# sudo ip netns add "ue11"

# sudo xterm -hold -e "$CURRENT_DIR/srsRAN_4G/build/srsue/src/srsue" "$CFG_PATH/ue12_zmq.conf"
# sleep 0.2
# sudo ip netns add "ue12"

# sudo xterm -hold -e "$CURRENT_DIR/srsRAN_4G/build/srsue/src/srsue" "$CFG_PATH/ue13_zmq.conf"
# sleep 0.2
# sudo ip netns add "ue13"
# Start UEs
for ue_id in $(seq 11 12); do
    sudo xterm -e "$CURRENT_DIR/srsRAN_4G/build/srsue/src/srsue" "$CFG_PATH/ue${ue_id}_zmq.conf" &
    pids+=($!)
    sleep 0.5
    sudo ip netns add "ue${ue_id}"
done

# sleep 6

# sudo xterm -hold -e "$CURRENT_DIR/build/broker"


# sleep 20

# for ue_id in $(seq 11 12); do
#     sudo ip netns exec "ue$P{ue_id}" ip route add default via 10.45.1.1 dev tun_srsue
#     sleep 0.2
# done

# echo "Press Enter to close all programs..."
# read enter_to_close


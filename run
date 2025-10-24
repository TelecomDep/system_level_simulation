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
    sudo xterm -hold -e "$1" -c "$2" &
    pids+=($!)
    sleep 0.2
}


# Start gNb
start_program "$CURRENT_DIR/srsRAN_Project/build/apps/gnb/gnb" "$CFG_PATH/gnb_zmq.yaml"

# Start UEs
for ue_id in $(seq 1 3); do
    sudo xterm -hold -e "$CURRENT_DIR/srsRAN_4G/build/srsue/src/srsue" "$CFG_PATH/ue${ue_id}_zmq.conf" &
    pids+=($!)
    sleep 0.2
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


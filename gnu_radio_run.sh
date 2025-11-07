#!/usr/bin/env bash

pids=()
CFG_PATH="$PWD/configs"
# USER_HOME="$HOME" # or... 
# USER_HOME="/home/<YOUR USER>" 
CURRENT_DIR=$(pwd)
LAUNCH_USER=$(whoami)
USER_HOME="/home/$(whoami)" 

NUM_UES=3

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


# Start UEs
for ue_id in $(seq 1 3); do
    sudo xterm -e "$CURRENT_DIR/srsRAN_4G/build/srsue/src/srsue" "$CFG_PATH/ue${ue_id}_zmq.conf" &
    pids+=($!)
    sleep 0.5
    sudo ip netns add "ue${ue_id}"
done


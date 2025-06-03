#!/usr/bin/env bash

pids=()
CFG_PATH="$PWD/configs"
# USER_HOME="$HOME" # or... 
# USER_HOME="/home/<YOUR USER>" 
CURRENT_DIR = "$PWD"
LAUNCH_USER=$(whoami)
USER_HOME="/home/$(whoami)" 

echo $CFG_PATH
echo $CURRENT_DIR
start_program() {
    sudo xterm -hold -e "$1" -c "$2" &
    pids+=($!)
    sleep 0.2
}


# Start Docker container for Open5GC
start_program "./$CURRENT_DIR/srsRAN_Project/build/apps/gnb/gnb" "$CFG_PATH/gnb_zmq.yaml"

# sudo xterm -hold -e "./$PWD/build/broker"
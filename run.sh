#!/usr/bin/env bash

pids=()
CFG_PATH="$PWD/cfg1"
# USER_HOME="$HOME" # or... 
# USER_HOME="/home/<YOUR USER>" 
LAUNCH_USER=$(whoami)
USER_HOME="/home/$(whoami)" 

echo $CFG_PATH
start_program() {
    sudo xterm -hold -e "$1" -c "$2" &
    pids+=($!)
    sleep 0.2
}

 sudo xterm -hold -e python3 "$PWD/tests/zmq_rep_req_srsran/broker.py"
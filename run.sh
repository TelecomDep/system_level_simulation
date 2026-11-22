#!/usr/bin/env bash

pids=()
CFG_PATH="$PWD/configs"
# USER_HOME="$HOME" # or... 
# USER_HOME="/home/<YOUR USER>" 
CURRENT_DIR=$(pwd)
LAUNCH_USER=$(whoami)
USER_HOME="/home/$(whoami)" 

NUM_UES=1

echo $CFG_PATH
echo $CURRENT_DIR
start_program() {
    sudo -E xterm -e "$1" "$2" &
    pids+=($!)
    sleep 0.2
}


# Start gNb
sudo -E xterm -e "$CURRENT_DIR/srsRAN_Project/build/apps/gnb/gnb" -c "$CFG_PATH/gnb_zmq.yaml" &
# Создаем новую сессию tmux и запускаем gNB в первом окне 
# tmux new-session -d -s $SESSION -n "gnb" "sudo $CURRENT_DIR/srsRAN_Project/build/apps/gnb/gnb $CFG_PATH/gnb_zmq.yaml"
pids+=($!)
sleep 1

# 2. Запуск ZMQ Брокера 
# echo "Starting Broker..." 
# sudo -E xterm -e "$CURRENT_DIR/zmq_broker/build/zmq_broker" "$CURRENT_DIR/zmq_broker/config.yaml" &
# pids+=($!)
# sleep 1

# Start UEs
for ue_id in $(seq 11 15); do
    sudo -E xterm -e "$CURRENT_DIR/srsRAN_4G/build/srsue/src/srsue" "$CFG_PATH/ue${ue_id}_zmq.conf" &
    # tmux new-window -t $SESSION -n "ue${ue_id}" "sudo $CURRENT_DIR/srsRAN_4G/build/srsue/src/srsue $CFG_PATH/ue${ue_id}_zmq.conf" &
    pids+=($!)
    sleep 1
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

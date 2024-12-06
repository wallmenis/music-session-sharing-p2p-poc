#!/bin/bash

RUN_PATH=$(pwd)
LOG_FILE_STUN="stun.log"
LOG_FILE_STUN_ERR="stun_err.log"
LOG_FILE_SIGNALING="SIGNALING.log"
# sudo -v
turnserver --no-auth --verbose 1>"$RUN_PATH/$LOG_FILE_STUN" 2>"$RUN_PATH/$LOG_FILE_STUN_ERR" &
# sudo nmcli device down enp1s0
trap : INT
tmux -c $RUN_PATH/signaling-server.py
kill $(pgrep turnserver)
# sudo nmcli device up enp1s0

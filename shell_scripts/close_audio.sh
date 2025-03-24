#!/bin/bash
# chmod 755 close_audio.sh

# Script to close a a data stream process

# Get process ID for the playing audio process
pid=$(pgrep FunctionPlayer)

if [[ ! -z $pid ]]; then
    echo "Terminating FunctionPlayer (PID: $pid)"
    kill $pid
else
    echo "Audio process not found"
fi
#!/bin/bash
# Usage: ./dyno.sh simpla     (device IP: PICO_IP variable or the default below)
IP=${PICO_IP:-192.168.15.125}
set -e
make "$1.bin" dynosend
./dynosend "$IP" "$1.bin"

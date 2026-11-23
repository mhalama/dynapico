#!/bin/bash
# Pouziti: ./dyno.sh simpla     (IP zarizeni: promenna PICO_IP nebo nize)
IP=${PICO_IP:-192.168.15.125}
set -e
make "$1.bin" dynosend
./dynosend "$IP" "$1.bin"

#!/bin/bash

PORT="/dev/ttyACM0"
MCU="m328p"
BAUD="115200"

PROJECT=$1

if [ -z "$PROJECT" ]; then
    echo "Usage: ./flash.sh project_name"
    exit 1
fi

HEX="build/$PROJECT/$PROJECT.hex"

if [ ! -f "$HEX" ]; then
    echo "Hex file not found: $HEX"
    exit 1
fi

echo "Flashing $HEX to ATmega328P..."

avrdude -c arduino -p $MCU -P $PORT -b $BAUD -U flash:w:$HEX:i

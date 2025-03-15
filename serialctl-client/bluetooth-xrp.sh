#!/bin/bash

MAC_ADDR="28:CD:C1:0B:11:6A"

sudo rfcomm unbind 0
sudo rfcomm bind 0 $MAC_ADDR

./serialctl /dev/rfcomm0 0

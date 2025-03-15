#!/bin/bash

echo "serialctl bluetooth helper"
echo ""
echo "Please select the device you want serialctl to communicate with."
echo "If the device is not paired, pair it and then restart."
# read bluetooth devices from bluetootctl
mapfile -t DEVICES < <(bluetoothctl devices)
# give the user a selection
select DEVICE in "${DEVICES[@]}"
do
  if [[ -z "$DEVICE" ]]; then
    echo "Invalid selection."
  else
    # extract MAC address from bluetoothctl line items
    MAC=$(echo $DEVICE | awk '{print $2}')

    # bind rfcomm device
    sudo rfcomm unbind 0
    sudo rfcomm bind 0 $MAC
    
    # run serialctl using the rfcomm device
    ./serialctl /dev/rfcomm0 0
    break
  fi
done

#!/bin/bash

module="dynmodul"
device="dynmodul"
mode="664"

#insmod dynmodule.ko

major=$(awk "\$2==\"$module\" {print \$1}" /proc/devices)

mknod /dev/${device}0 c $major 0
mknod /dev/${device}1 c $major 1

ln -sf ${device}0 /dev/${device}

chgrp staff /dev/${device}[0-1]
chmod $mode /dev/${device}[0-1]

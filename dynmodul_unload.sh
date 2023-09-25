#!/bin/sh

device="dynmodul"

sudo rmmod dynmodule.ko 

unlink /dev/dynmodul
rm -f /dev/${device}[0-1]

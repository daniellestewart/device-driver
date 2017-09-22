#!/bin/sh

# CSCI 5103 Fall 2016
# Assignment # 7
# name: Alexander Cina, Danielle Stewart
# student id: 4515522, 4339650 
# x500 id: cinax020, dkstewar
# CSELABS machine: csel-x07-umh and csel-x34-umh

# This is a script for removing and uninastalling
# the scullbuffer device driver.

module="scullbuffer"
device="scullbuffer"

# invoke rmmod with all arguments we got
/sbin/rmmod $module $* || exit 1

# Remove stale nodes

rm -f /dev/${device} /dev/${device}
rm -f /dev/${device}priv
rm -f /dev/${device}single
rm -f /dev/${device}uid
rm -f /dev/${device}wuid

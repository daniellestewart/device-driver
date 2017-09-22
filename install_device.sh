#!/bin/sh

# CSCI 5103 Fall 2016
# Assignment # 7
# name: Alexander Cina, Danielle Stewart
# student id: 4515522, 4339650
# x500 id: cinax020, dkstewar
# CSELABS machine: csel-x07-umh and csel-x34-umh
# 
# This is a file for installing the scullbuffer
# device driver into the kernel.
#
# Modified by Cina/Stewart to be scullbuffer install_device.sh

# $Id: scull_load,v 1.4 2004/11/03 06:19:49 rubini Exp $
module="scullbuffer"
device="scullbuffer"
mode="777"


# invoke insmod with all arguments we got
# and use a pathname, as insmod doesn't look in . by default
/sbin/insmod ./$module.ko $* || exit 1

# retrieve major number
major=$(awk "\$2==\"$module\" {print \$1}" /proc/devices)
echo "major='$major'"
minor=1
echo "minor='$minor'"

# Remove stale nodes and replace them, then give gid and perms
# Usually the script is shorter, it's scull that has several devices in it.

rm -f /dev/${device}1
mknod /dev/${device}1 c $major $minor
ln -sf ${device}1 /dev/${device}
chmod $mode  /dev/${device}1

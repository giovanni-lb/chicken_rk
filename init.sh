#!/bin/bash

# compile the project
cd src/ && make clean && make install

# copy driver to persistance location
mkdir /lib/modules/5.19.0-41-generic/kernel/drivers/chicken_rk
cp chicken_rk.ko /lib/modules/5.19.0-41-generic/kernel/drivers/chicken_rk/

echo "chicken_rk" > /etc/modules-load.d/chicken_rk.conf

# Update kernel modules dependencies
depmod -a

# /etc/modules coutain the list of module that will be load at boot time
echo "chicken_rk" >> /etc/modules



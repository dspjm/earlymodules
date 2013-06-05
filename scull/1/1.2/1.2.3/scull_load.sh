#!/bin/bash
dev_cnt=4
insmod scull.ko
major=`gawk '$2 ~ /^scull$/ {print $1}' /proc/devices | grep -m 1 .`
for (( i=0 ; i<dev_cnt ; i++ )) ; do
mknod -m 666 /dev/scull${i} c ${major} ${i}
done

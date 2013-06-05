#!/bin/bash 
DEV_NUM=5
dev_name=umdm_cdev
insmod umdm.ko
#major=`awk "\$2 ~ /^${dev_name}\$/ { print \$1 }" /proc/devices`
major=$(awk '$2 ~ /^'$dev_name'$/ {print $1}' /proc/devices)
if [ "$major" == '' ] ; then
echo "can't find major number"
exit 1
fi
for (( i=0 ; $i<$DEV_NUM ; i++ )) ; do
mknod /dev/umdm${i} c $major $i
done


#!/bin/bash
dev_cnt=4
for (( i=0 ; i<dev_cnt ; i++ )) ; do
rm -rf /dev/scull${i}
done
rmmod scull

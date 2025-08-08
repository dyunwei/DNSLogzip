#!/bin/bash

#
# Convert tab delimiters to spaces.
#

pip3 install tqdm scipy textdistance


cd /media/ramdisk/data/ISP-23/
expand -t 1 Log.txt > ISP-23.log

cd /media/ramdisk/data/ISP-24/
expand -t 1 Log.txt > ISP-24.log

cd /media/ramdisk/data/Public/
expand -t 1 Log.txt > Public.log

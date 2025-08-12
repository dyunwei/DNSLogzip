#!/bin/bash

#
# We convert tabs to spaces because some advanced compression algorithms do not support using tabs as field delimiters.
#

cd /media/ramdisk/data/ISP-23/
expand -t 1 Log.txt > ISP-23.log

cd /media/ramdisk/data/ISP-24/
expand -t 1 Log.txt > ISP-24.log

cd /media/ramdisk/data/Public/
expand -t 1 Log.txt > Public.log

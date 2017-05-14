#!/bin/sh

name="serp"

sudo make clean
sudo make
#sudo gcc test.c -o test

#if [ -f /dev/echo ]; then
sudo ./unload.sh $name
#fi

if [ $# ">" 0 ]; then
    sudo ./load.sh $name echo_numdevs="$1"
else
    sudo ./load.sh $name
fi

#sudo ./test /dev/$name

exit 0

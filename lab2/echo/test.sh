#!/bin/sh

name="echo"

sudo make clean
sudo make
sudo gcc test.c -o test

#if [ -f /dev/echo ]; then
    sudo ./unload.sh $name
#fi

if [ $# ">" 1 ]; then
    sudo ./load.sh $name $1
else
    sudo ./load.sh $name
fi

sudo ./test /dev/$name

exit 0

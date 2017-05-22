#!/bin/sh

name="seri"

sudo make clean
sudo make
sudo gcc test.c -o test

sudo ./unload.sh $name

if [ $# ">" 0 ]; then
    sudo ./load.sh $name serp_numdevs="$1"
else
    sudo ./load.sh $name
fi

sudo ./test /dev/$name

exit 0

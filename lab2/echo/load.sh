#! /bin/sh

privileges="666"

#Loading code
if [ $# "<" 1 ]; then
	echo "Usage: $0 <module_name>"
	exit -1
else
	module="$1"
	echo "Module Device: $1"
	
	if [ $# ">" 1 ]; then
        insmod -f ./${module}.ko $2 || exit 1
	else
        insmod -f ./${module}.ko || exit 1
	fi
fi


major=`cat /proc/devices | awk "\\$2==\"$module\" {print \\$1}"| head -n 1`
mknod /dev/${module} c $major 0

chmod $privileges /dev/${module}

exit 0

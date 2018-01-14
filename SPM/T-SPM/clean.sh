#!/bin/bash
	sudo ./unload.sh
	sudo cat /dev/null > /var/log/kern.log
	sudo dmesg --clear
	sudo ./load.sh

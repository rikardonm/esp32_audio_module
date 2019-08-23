#!/bin/bash

# an executable wrapper so that eclipse can call make
DIR="$( cd "$( dirname "#{BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# just to make sure
echo "FUCK"
echo $USER

echo $@



#dev_code=067b:2303
#device:="/dev/bus/usb/$(shell lsusb -d $(dev_code) | cut -d: -f1 | cut -d\  -f2,4 | tr ' ' '/')"


if test "$(ls /dev/ | grep ttyUSB0)"
then
	echo "found"
	ttydevice="--device /dev/ttyUSB0"
fi




function call_docker {
	docker run \
		--rm \
		$3 \
		-v $(pwd):/app/ \
		-w="/app/$2/" \
		--env IDF_PATH="/app/src/esp-idf/" \
		$ttydevice \
		esp32 \
		$1
}


function print_help {
	echo "Supports:"
	echo "- build"
	echo "- prepare_docker_wo_sudo"
	echo "- shell"
	echo "- sinker"
	echo "- src/esp-idf"
	echo "- update_idf"
	echo "- compiler"
}

cmd=$1

case "$cmd" in
	update_idf)
		# copy frameworks and libraries
		# not used by the docker image, and actually this directory is ignore in .gitignore
		# but it is an optional step so that we can browse the library source code
		# and have meaningful code completion on IDEs
		cd src && git pull --recurse-submodules
		;;
	help)
		print_help
		;;
	compiler)
		if test ! "$(ls ./xtensa-esp32-elf)"
		then
			# install compiler
			wget https://dl.espressif.com/dl/xtensa-esp32-elf-gcc8_2_0-esp32-2019r1-linux-amd64.tar.gz -O idf.tar.gz
			mkdir xtensa-esp32-gcc
			tar -xzf idf.tar.gz
			rm idf.tar.gz
		fi
		;;
	build)
		docker build -t esp32 -f Dockerfile .
		;;
	shell)
		call_docker bash . -it
		;;
	sinker)
		call_docker make src/sinker
		;;
	prepare_docker_wo_sudo)
		sudo groupadd docker
		sudo gpasswd -a $USER docker
		;;
	*)
		echo "No argument given/ not found."
		print_help
		;;
esac



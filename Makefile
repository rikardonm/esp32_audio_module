
cdir=$(shell pwd)

#dev_code=067b:2303
#device:="/dev/bus/usb/$(shell lsusb -d $(dev_code) | cut -d: -f1 | cut -d\  -f2,4 | tr ' ' '/')"

ttydevice=
ifeq ($(shell ls /dev/ | grep ttyUSB0),)
else
	ttydevice="--device /dev/ttyUSB0"
endif


# copy frameworks and libraries
# not used by the docker image, and actually this directory is ignore in .gitignore
# but it is an optional step so that we can browse the library source code
# and have meaningful code completion on IDEs
src/esp-idf:
	@git clone --recursive https://github.com/espressif/esp-idf.git


.PHONY: update_idf
update_idf: src/esp-idf
	@cd src && git pull --recursive-submodules

.PHONY: build
build: src/esp-idf
	@sudo docker build -t esp32 -f Dockerfile .



.PHONY: shell
shell:
	@sudo docker run \
		-it \
		--rm \
		-v $(cdir):/app/ \
		-w="/app/" \
		--env IDF_PATH="/app/src/esp-idf/" \
		$(ttydevice) \
		esp32 \
		bash


.PHONY: help
help:
	@echo "Supports:"
	@echo "- build"
	@echo "- shell"
	@echo "- src/esp-idf"
	@echo "- update_idf"


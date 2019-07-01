.DEFAULT_GOAL:= sinker

cdir=$(shell pwd)

#dev_code=067b:2303
#device:="/dev/bus/usb/$(shell lsusb -d $(dev_code) | cut -d: -f1 | cut -d\  -f2,4 | tr ' ' '/')"

ttydevice=
ifeq ($(shell ls /dev/ | grep ttyUSB0),)
else
	ttydevice="--device /dev/ttyUSB0"
endif




define docker_cmd
	@docker run \
		-it \
		--rm \
		-v $(cdir):/app/ \
		-w="/app/$(2)/" \
		--env IDF_PATH="/app/src/esp-idf/" \
		$(ttydevice) \
		esp32 \
		$(1)

endef




# copy frameworks and libraries
# not used by the docker image, and actually this directory is ignore in .gitignore
# but it is an optional step so that we can browse the library source code
# and have meaningful code completion on IDEs


.PHONY: update_idf
update_idf: src/esp-idf
	@cd src && git pull --recursive-submodules

.PHONY: build
build: src/esp-idf
	@sudo docker build -t esp32 -f Dockerfile .


.PHONY: prepare_docker_wo_sudo
prepare:
	@sudo groupadd docker
	@sudo gpasswd -a $USER docker


.PHONY: shell
shell:
	$(call docker_cmd, bash)

.PHONY: sinker
sinker:
	$(call docker_cmd,make,src/sinker)




.PHONY: help
help:
	@echo "Supports:"
	@echo "- build"
	@echo "- prepare_docker_wo_sudo"
	@echo "- shell"
	@echo "- sinker"
	@echo "- src/esp-idf"
	@echo "- update_idf"


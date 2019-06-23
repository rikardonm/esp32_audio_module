FROM ubuntu:18.04


RUN apt-get update && apt-get install -y \
	vim \
	wget \
	curl \
	gcc \
	git \
	wget \
	make \
	libncurses-dev \
	flex \
	bison \
	gperf \
	python \
	python-pip \
	python-setuptools \
	python-serial \
	python-cryptography \
	python-future \
	python-pyparsing \
	python-pyelftools



# install compiler
RUN mkdir /tools
WORKDIR /tools
RUN wget https://dl.espressif.com/dl/xtensa-esp32-elf-gcc8_2_0-esp32-2019r1-linux-amd64.tar.gz \
	-O idf.tar.gz

RUN tar -xzf idf.tar.gz
RUN rm idf.tar.gz

ENV PATH="/tools/xtensa-esp32-elf/bin:${PATH}"

# set up idf library
WORKDIR /tools
RUN git clone --recursive https://github.com/espressif/esp-idf.git
ENV IDF_PATH="/tools/esp-idf"
RUN python -m pip install --user -r $IDF_PATH/requirements.txt

# set up user location
RUN mkdir /app

# just drop into shell by default
CMD ["/bin/bash"]


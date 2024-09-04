CSDK_PLATFORM_WRAPPER_INC=/home/markku/Nuance/csdk-v6-linux_x86_64/csdk/sample/linux/inc
CSDK_PLATFORM_WRAPPER_SRC=/home/markku/Nuance/csdk-v6-linux_x86_64/csdk/sample/linux/src
UTILS = ~/cJSON/libcjson.so
INCLUDES = -I ~/cJSON -I $(CSDK_PLATFORM_WRAPPER_INC)
DIR_BIN = bin
#LIBS = $(shell pkg-config --libs libevdev)
#INCLUDES = $(shell pkg-config --cflags libevdev)

build: create_dirs
	$(CC) $(LIBS) $(INCLUDES) -pthread -o bin/biom_testapp src/actionMain.c src/mosquitto.c src/util.c src/action.c $(CSDK_PLATFORM_WRAPPER_SRC)/mt_mutex.c $(CSDK_PLATFORM_WRAPPER_SRC)/mt_semaphore.c $(UTILS) -Lbin -lrt -lmosquitto


create_dirs:
	@if [ ! -d "$(DIR_BIN)" ]; then mkdir -p $(DIR_BIN); fi


clean:
	@if [ -d "$(DIR_BIN)" ]; then rm -rf $(DIR_BIN); fi

all: clean build

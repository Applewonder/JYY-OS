export TOKEN := Cxcg92cm
export MODULE  := L1
NAME           := kernel
SRCS           := framework/main.c $(shell find -L ./src/ -name "*.c")
INC_PATH       := include/ framework/

CFLAGS += -g

export AM_HOME := $(PWD)/../abstract-machine
ifeq ($(ARCH),)
export ARCH := x86_64-qemu
endif

include $(AM_HOME)/Makefile
include ../Makefile.lab
image: git

test: git
		@gcc $(shell find src/ -name "*.c")  \
				$(shell find test/ -name "*.c") \
				-g\
				-Iframework -Itest -Iinclude -DTEST -lpthread\
				-o build/test
runtest: 
		@build/test
debug: 
	./debug.sh
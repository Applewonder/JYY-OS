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
				-fsanitize=thread \
				-g\
				-Iframework -Itest -Iinclude -DTEST -lpthread\
				-o build/test
runtest: 
		@build/test

testall: test
		@build/test 0
		@build/test 1
		@build/test 2
		@build/test 3
		@build/test 4
		@build/test 5

testcase0: test
		@build/test 0

testcase1: test
		@build/test 1

testcase2: test
		@build/test 2

testcase3: test
		@build/test 3

testcase4: test
		@build/test 4

testcase5: test
		@build/test 5

testcase6: test
		@build/test 6

testcase7: test
		@build/test 7

debug: 
	./debug.sh
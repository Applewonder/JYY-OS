include $(AM_HOME)/scripts/isa/x86_64.mk
include $(AM_HOME)/scripts/platform/qemu.mk

AM_SRCS := x86/qemu/start64.S \
           x86/qemu/trap64.S \
           x86/qemu/trm.c \
           x86/qemu/cte.c \
           x86/qemu/ioe.c \
           x86/qemu/vme.c \
           x86/qemu/mpe.c

run: build-arg
	@qemu-system-x86_64 $(QEMU_FLAGS)

debug: build-arg
	@qemu-system-x86_64 -s -S $(QEMU_FLAGS)

testdev: build-arg
	@qemu-system-x86_64 $(QEMU_FLAGS)

testprint: build-arg
	@qemu-system-x86_64 $(QEMU_FLAGS)

testpv: build-arg
	@qemu-system-x86_64 $(QEMU_FLAGS)

gdbprint: build-arg
	@qemu-system-x86_64 -s -S $(QEMU_FLAGS)

gdbpv: build-arg
	@qemu-system-x86_64 -s -S $(QEMU_FLAGS)

testnormal: build-arg
	@qemu-system-x86_64 $(QEMU_FLAGS)

gdbnormal: build-arg
	@qemu-system-x86_64 -s -S $(QEMU_FLAGS)

teststarve: build-arg
	@qemu-system-x86_64 $(QEMU_FLAGS)

gdbstarve: build-arg
	@qemu-system-x86_64 -s -S $(QEMU_FLAGS)


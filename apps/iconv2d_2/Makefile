CC     := riscv64-unknown-elf-gcc
SPIKE_BIN  := /home/annalia/DoE/install/riscv-isa-sim/bin/spike
PK     := /home/annalia/RISC-V/rv64/pk/riscv64-unknown-elf/bin/pk


MARCH  := rv64gcv_zicsr_zicntr
ABI    := lp64


CFLAGS := -O2 -march=$(MARCH) -mabi=$(ABI)
VLEN ?= 256


ifeq ($(SPIKE),1)
    CFLAGS += -DSPIKE
endif


TARGET := main.elf
SRCS   := main.c src/iconv2d_3x3.c src/iconv2d_5x5.c src/iconv2d_7x7.c ../common/util.c inc/dataset.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^


run: $(TARGET)
ifeq ($(SPIKE),1)
	$(SPIKE_BIN) --isa=$(MARCH) --varch="vlen:$(VLEN),elen:64" $(PK) ./$(TARGET)
else
	@echo "Esecuzione standard (es. Verilator)"
	./$(TARGET)
endif

clean:
	$(RM) $(TARGET)

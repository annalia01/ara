
# ==== Toolchain e percorsi ====
CC          := riscv64-unknown-elf-gcc
SPIKE_BIN   := /home/annalia/DoE/install/riscv-isa-sim/bin/spike
PK          := /home/annalia/RISC-V/rv64/pk/riscv64-unknown-elf/bin/pk

# ==== ISA/ABI ====
MARCH       := rv64gcv_zicsr_zicntr
ABI         := lp64
VLEN        ?= 256

# ==== Flag di compilazione ====
CFLAGS      := -O2 -march=$(MARCH) -mabi=$(ABI) -DSPIKEGEM

# ==== Target e sorgenti ====
TARGET      := main.elf
SRCS        := main.c src/iconv2d_3x3.c src/iconv2d_5x5.c src/iconv2d_7x7.c ../common/util.c inc/dataset.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^


run: $(TARGET)
	$(SPIKE_BIN) --isa=$(MARCH) --varch="vlen:$(VLEN),elen:64" $(PK) ./$(TARGET)

clean:
	$(RM) $(TARGET)

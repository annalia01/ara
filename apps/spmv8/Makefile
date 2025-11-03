# ==== Toolchain e percorsi ====
CC          := riscv64-unknown-elf-gcc
SPIKE_BIN   := /home/annalia/DoE/install/riscv-isa-sim/bin/spike
PK          := /home/annalia/RISC-V/rv64/pk/riscv64-unknown-elf/bin/pk

# ==== ISA/ABI ====
MARCH       := rv64gcv_zicsr_zicntr
ABI         := lp64
VLEN        ?= 256

# ==== Flag di compilazione ====
CFLAGS      := -O2 -march=$(MARCH) -mabi=$(ABI) -DSPIKE

# ==== Target e sorgenti ====
TARGET      := main.elf
SRCS        := main.c src/spmv.c ../common/util.c inc/dataset.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRCS)
	@echo "→ Compilazione con flag SPIKE=1"
	$(CC) $(CFLAGS) -o $@ $^


run: $(TARGET)
	@echo "→ Avvio simulazione Spike..."
	$(SPIKE_BIN) --isa=$(MARCH) --varch="vlen:$(VLEN),elen:64" $(PK) ./$(TARGET)

clean:
	$(RM) $(TARGET)

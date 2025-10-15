# ==== Toolchain e percorsi ====
CC     := riscv64-unknown-elf-gcc
SPIKE  := /home/annalia/RISC-V/rv64/spike/bin/spike
PK     := /home/annalia/RISC-V/rv64/pk/riscv64-unknown-elf/bin/pk

# ==== ISA/ABI ====
MARCH  := rv64gcv_zicsr_zicntr
ABI    := lp64

# ==== Flag minimi ====
CFLAGS := -O2 -march=$(MARCH) -mabi=$(ABI)

# ==== Sorgenti/target ====
TARGET := main.elf
SRCS   := main.c src/iconv2d_3x3.c ../../common/util.c inc/dataset.c src/iconv2d_5x5.c src/iconv2d_7x7.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

run: $(TARGET)
	$(SPIKE) --isa=$(MARCH) $(PK) ./$(TARGET)

clean:
	$(RM) $(TARGET)

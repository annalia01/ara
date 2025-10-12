

# --- CUSTOM CONFIGURATION: usa sempre GCC ---
COMPILER = gcc

# Usa la toolchain GCC bare-metal (installata in install/riscv-gcc)
RISCV_PREFIX  ?= $(GCC_INSTALL_DIR)/bin/
RISCV_CC      ?= $(RISCV_PREFIX)$(RISCV_TARGET)-gcc
RISCV_CXX     ?= $(RISCV_PREFIX)$(RISCV_TARGET)-g++
RISCV_AS      ?= $(RISCV_PREFIX)$(RISCV_TARGET)-as
RISCV_AR      ?= $(RISCV_PREFIX)$(RISCV_TARGET)-ar
RISCV_LD      ?= $(RISCV_PREFIX)$(RISCV_TARGET)-ld
RISCV_OBJDUMP ?= $(RISCV_PREFIX)$(RISCV_TARGET)-objdump
RISCV_OBJCOPY ?= $(RISCV_PREFIX)$(RISCV_TARGET)-objcopy
RISCV_STRIP   ?= $(RISCV_PREFIX)$(RISCV_TARGET)-strip

# Architettura e ABI coerenti con Spike
RISCV_ARCH ?= rv64gcv_zicsr_zicntr
RISCV_ABI  ?= lp64d
# =====================================================================


# Copyright 2020 ETH Zurich and University of Bologna.
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Author: Samuel Riedel, ETH Zurich
#         Matheus Cavalcante, ETH Zurich
SHELL = /usr/bin/env bash

ROOT_DIR := $(patsubst %/,%, $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
ARA_DIR := $(shell git rev-parse --show-toplevel 2>/dev/null || echo $$ARA_DIR)

# Choose Ara's configuration
ifndef config
	ifdef ARA_CONFIGURATION
		config := $(ARA_CONFIGURATION)
	else
		config := default
	endif
endif

# Include configuration
include $(ARA_DIR)/config/$(config).mk

INSTALL_DIR             ?= $(ARA_DIR)/install
GCC_INSTALL_DIR         ?= $(INSTALL_DIR)/riscv-gcc
LLVM_INSTALL_DIR        ?= $(INSTALL_DIR)/riscv-llvm
ISA_SIM_INSTALL_DIR     ?= $(INSTALL_DIR)/riscv-isa-sim
ISA_SIM_MOD_INSTALL_DIR ?= $(INSTALL_DIR)/riscv-isa-sim-mod

RISCV_XLEN    ?= 64
RISCV_TARGET  ?= riscv$(RISCV_XLEN)-unknown-elf

# Use gcc to compile scalar riscv-tests
RISCV_CC_GCC  ?= $(GCC_INSTALL_DIR)/bin/$(RISCV_TARGET)-gcc

# Benchmark with spike
spike_env_dir ?= $(ARA_DIR)/apps/riscv-tests
SPIKE_INC     ?= -I$(spike_env_dir)/env -I$(spike_env_dir)/benchmarks/common
SPIKE_CCFLAGS ?= -DPREALLOCATE=1 -DSPIKE=1 $(SPIKE_INC)
SPIKE_LDFLAGS ?= -nostdlib -T$(spike_env_dir)/benchmarks/common/test.ld
RISCV_SIM     ?= $(ISA_SIM_INSTALL_DIR)/bin/spike
RISCV_SIM_MOD ?= $(ISA_SIM_MOD_INSTALL_DIR)/bin/spike

# Limit vector length for Spike (max 4096)
vlen_spike := $(shell vlen=$$(grep vlen $(ARA_DIR)/config/$(config).mk | cut -d" " -f3) && echo "$$(( $$vlen < 4096 ? $$vlen : 4096 ))")
RISCV_SIM_OPT ?= --isa=$(RISCV_ARCH)_zfh --varch="vlen:$(vlen_spike),elen:64"
RISCV_SIM_MOD_OPT ?= --isa=$(RISCV_ARCH)_zfh --varch="vlen:$(vlen_spike),elen:64" -d

# Python
PYTHON ?= python3

# Defines

MAKE_DEFINES = -DNR_LANES=$(nr_lanes) -DVLEN=$(vlen)
DEFINES += $(ENV_DEFINES) $(MAKE_DEFINES)

# Common warnings
RISCV_WARNINGS += -Wunused-variable -Wall -Wextra -Wno-unused-command-line-argument

# GCC Flags
RISCV_FLAGS_GCC    ?= -mcmodel=medany -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) \
                      -I$(CURDIR)/common -static -O3 -ffast-math -fno-common \
                      -fno-builtin-printf $(DEFINES) $(RISCV_WARNINGS)

RISCV_CCFLAGS_GCC  ?= $(RISCV_FLAGS_GCC) -std=gnu99
RISCV_CXXFLAGS_GCC ?= $(RISCV_FLAGS_GCC)
RISCV_LDFLAGS_GCC  ?= -static -nostartfiles -lm -lgcc $(RISCV_FLAGS_GCC) \
                      -std=gnu99 -T$(CURDIR)/common/link.ld

# Runtime objects (bare-metal)
RUNTIME_GCC   ?= common/crt0-gcc.S.o common/printf-gcc.c.o common/string-gcc.c.o \
                 common/serial-gcc.c.o common/util-gcc.c.o
RUNTIME_SPIKE ?= $(spike_env_dir)/benchmarks/common/crt.S.o.spike \
                 $(spike_env_dir)/benchmarks/common/syscalls.c.o.spike \
                 common/util.c.o.spike

.INTERMEDIATE: $(RUNTIME_GCC)

# Compilation rules (generic)
%-gcc.S.o: %.S
	$(RISCV_CC_GCC) $(RISCV_CCFLAGS_GCC) -c $< -o $@

%-gcc.c.o: %.c
	$(RISCV_CC_GCC) $(RISCV_CCFLAGS_GCC) -c $< -o $@

%.S.o: %.S
	$(RISCV_CC) $(RISCV_CCFLAGS_GCC) -c $< -o $@

%.c.o: %.c
	$(RISCV_CC) $(RISCV_CCFLAGS_GCC) -c $< -o $@

%.cpp.o: %.cpp
	$(RISCV_CXX) $(RISCV_CXXFLAGS_GCC) -c $< -o $@

%.ld: %.ld.c
	$(RISCV_CC_GCC) -P -E $(DEFINES) $< -o $@

# =====================================================================
# End of runtime.mk
# =====================================================================

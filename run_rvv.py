import os
import m5
from m5.objects import (
    System, SrcClockDomain, VoltageDomain, Root,
    RiscvO3CPU, Cache, AddrRange, SEWorkload, Process,
    MemCtrl, DDR3_1600_8x8, SystemXBar, L2XBar,
    RiscvISA, TimingSimpleCPU, MinorCPU
)

# ==================== CACHE MODELS ====================

class L1Cache(Cache):
    assoc = 2
    tag_latency = 2
    data_latency = 2
    response_latency = 2
    mshrs = 4
    tgts_per_mshr = 20

    def connectCPU(self, cpu):
        raise NotImplementedError

    def connectBus(self, bus):
        self.mem_side = bus.cpu_side_ports

class L1ICache(L1Cache):
    size = "16kB"
    def connectCPU(self, cpu):
        self.cpu_side = cpu.icache_port

class L1DCache(L1Cache):
    size = "64kB"
    def connectCPU(self, cpu):
        self.cpu_side = cpu.dcache_port

class L2Cache(Cache):
    size = "256kB"
    assoc = 8
    tag_latency = 20
    data_latency = 20
    response_latency = 20
    mshrs = 20
    tgts_per_mshr = 12
    def connectCPUSideBus(self, bus):
        self.cpu_side = bus.mem_side_ports
    def connectMemSideBus(self, bus):
        self.mem_side = bus.cpu_side_ports

# ==================== SYSTEM CONFIGURATION ====================

system = System()
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "1GHz"
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = "timing"
system.mem_ranges = [AddrRange("512MB")]


system.cpu = MinorCPU()

# ===== RVV CONFIGURATION =====
# Legge i parametri da variabili d'ambiente (o usa default)
vlen = int(os.environ.get("VLEN", "256"))
elen = int(os.environ.get("ELEN", "64"))
print(f"[gem5] Configurazione RVV: VLEN={vlen}, ELEN={elen}")

system.cpu.isa = RiscvISA(vlen=vlen, elen=elen)
# =============================

# L1 cache
system.cpu.icache = L1ICache()
system.cpu.dcache = L1DCache()
system.cpu.icache.connectCPU(system.cpu)
system.cpu.dcache.connectCPU(system.cpu)

# L2 + Bus
system.l2bus = L2XBar()
system.cpu.icache.connectBus(system.l2bus)
system.cpu.dcache.connectBus(system.l2bus)

system.l2cache = L2Cache()
system.l2cache.connectCPUSideBus(system.l2bus)

system.membus = SystemXBar()
system.l2cache.connectMemSideBus(system.membus)

system.system_port = system.membus.cpu_side_ports

system.cpu.createInterruptController()

system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports

# ==================== WORKLOAD ====================


binary = "/home/annalia/DoE/apps/spmv/main.elf"

# Imposta workload SE
system.workload = SEWorkload.init_compatible(binary)

process = Process()
process.cmd = [binary]
system.cpu.workload = process

# Crea i thread DOPO aver assegnato l’ISA
system.cpu.createThreads()

# ==================== SIMULATION RUN ====================

root = Root(full_system=False, system=system)
m5.instantiate()

print("Inizio simulazione RVV...")
exit_event = m5.simulate()

print(f"Uscita @ tick {m5.curTick()} per motivo: {exit_event.getCause()}")

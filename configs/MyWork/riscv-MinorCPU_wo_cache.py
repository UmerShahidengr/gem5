# import the m5 library and all SimObjects that we’ve compiled.
import m5
from m5.objects import *

# create the first SimObject: the system 
# The System object will be the parent of all the other objects in our simulated system. 
# The System object contains a lot of functional (not timing-level) information, 
# like the physical memory ranges, the root clock domain, the root voltage domain, 
# the kernel (in full-system simulation), etc. 

# To create the system SimObject, we simply instantiate it like a normal python class
system = System()


# Now that we have a reference to the system we are going to simulate, 
# e.g, setting clock on the system, set the clock frequency on that domain, 
# voltage domain for this clock domain etc. 
system.clk_domain = SrcClockDomain()			# default value
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()	# default value

# Set up memory
system.mem_mode = 'timing'
system.mem_ranges = [AddrRange('512MB')]

# Create CPU
system.cpu = MinorCPU()

# create the system-wide memory bus
system.membus = SystemXBar()

# Set up cache bus ; if no cache, then do this:
system.cpu.icache_port = system.membus.cpu_side_ports
system.cpu.dcache_port = system.membus.cpu_side_ports


# Connect the interrupt controller
system.cpu.createInterruptController()
system.system_port = system.membus.cpu_side_ports

# Connect memory controller
system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports


# Choose binary file to run (In this case, basic hello world example is selected
binary = 'gem5-resources/src/riscv-tests/benchmarks/rsort.riscv'

# for gem5 V21 and beyond
system.workload = SEWorkload.init_compatible(binary)


# THe next step is to create the process (another SimObject). 
# Then we set the processes command to the command we want to run. 
# Then we set the CPU to use the process as it’s workload, 
# and finally create the functional execution contexts in the CPU.
process = Process()
process.cmd = [binary]
system.cpu.workload = process
system.cpu.createThreads()


# The final thing we need to do is instantiate the system and begin execution. 
# First, we create the Root object. 
# Then we instantiate the simulation. 
# The instantiation process goes through all of the SimObjects we’ve created in python and creates the C++ equivalents.
root = Root(full_system = False, system = system)
m5.instantiate()


# kick off the actual simulation!
print("Beginning simulation!")
exit_event = m5.simulate()

# And once simulation finishes, we can inspect the state of the system.
print('Exiting @ tick {} because {}'
      .format(m5.curTick(), exit_event.getCause()))

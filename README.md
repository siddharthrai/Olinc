# Olinc (Offline cache)

Olinc is a functional memory hierarchy simulator for last-level cache (LLC) and DRAM. 

Usage:
------
Simulator expects a configuration file describing cache configuration and DRAM timing 
parameters and an input memory trace for execution. Memory trace format recognized is
described in the <bold>export-trace/export.h</bold> header file. A reader utility present in the "export-trace" directory can be used to read a valid trace file.

Configuration files:
-------------------
Configuration files for the simulator are available in the "config" directory. Before start of the 
simulation, both cache and dram configuration file should be copied to the current working 
directory.

Build:
----------
To build the simulator, user need to run make on the top-level directory. Make will generate the 
cache-sim binary in the bin directory.

Execution:
----------
Simulator can be executing by supplying the simulator configuration and the memory trace file 
as command line arguments as follows:

./cache-sim -f "config-file-name" -t "memory-trace-file-name"

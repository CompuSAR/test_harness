
CXXFLAGS=-Wall -Wextra -std=c++20 -g
CC=$(CXX)

simulate_mos: simulate_mos.o readmem.o cpu/perfect6502.o cpu/netlist_sim.o

readmem.o: readmem.h
simulate_mos.o: readmem.h
cpu/%.o:
	$(MAKE) -C cpu $*.o

clean:
	$(RM) *.o simulate_mos
.PHONY: clean

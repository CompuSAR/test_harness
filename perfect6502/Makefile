
CXXFLAGS=-Wall -Wextra -std=c++20 -g
CC=$(CXX)

test_plan_mos.mem: simulate_mos ../test_program/test_program_mos.mem
	./simulate_mos ../test_program/test_program_mos.mem > $@.tmp
	mv $@.tmp $@

simulate_mos: simulate_mos.o readmem.o cpu/perfect6502.o cpu/netlist_sim.o

readmem.o: readmem.h
simulate_mos.o: readmem.h
cpu/%.o:
	$(MAKE) -C cpu $*.o

../test_program/%:
	$(MAKE) -C ../test_program "$*" CPU=mos

clean:
	$(RM) *.o simulate_mos *.mem
	$(MAKE) -C cpu clean
	$(MAKE) -C ../test_program clean
.PHONY: clean

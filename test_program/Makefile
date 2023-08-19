ASM=vasm6502_oldstyle

CPU=wdc

BASEDIR=.

ifeq "$(CPU)" "wdc"
	CPU_OPTIONS:=-wdc02 -DCPU_WDC=1 -DC02=1 -DCPU_Illegal=0
endif

ifeq "$(CPU)" "mos"
	CPU_OPTIONS:=-illegal -DCPU_WDC=0 -DC02=0 -DCPU_Illegal=1
endif

test_program_$(CPU).mem:

-include *.dep

%_$(CPU).lst: %.s
	$(ASM) -x -dotdir $(CPU_OPTIONS) -dependall=make -depfile "$*_$(CPU).dep" -L "$*_$(CPU).lst" "$<" -o "$*_$(CPU).out"
	sed -i -e 's/$*_$(CPU)\.bin/\0 $*_$(CPU).lst/' "$*_$(CPU).dep"

.PRECIOUS: %_$(CPU).lst

%.mem: %.lst
	$(BASEDIR)/parse_lst "$<" "$@".tmp
	mv "$@".tmp "$@"

clean:
	$(RM) *.lst *.dep *.bin *.mem *.out

.PHONY: clean

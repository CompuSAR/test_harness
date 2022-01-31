ASM=vasm6502_oldstyle

CPU=wdc

ifeq "$(CPU)" "wdc"
	CPU_OPTIONS:=-wdc02 -DCPU_WDC
endif

test_program_$(CPU).lst:

-include *.dep

%_$(CPU).lst: %.s
	$(ASM) -dotdir $(CPU_OPTIONS) -dependall=make -depfile "$*_$(CPU).dep" -L "$*_$(CPU).lst" "$<" -Fbin -o "$*_$(CPU).bin"
	sed -i -e 's/$*_$(CPU)\.bin/\0 $*_$(CPU).lst/' "$*_$(CPU).dep"


clean:
	$(RM) *.lst *.dep *.bin

.PHONY: clean

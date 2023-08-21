#include "readmem.h"

extern "C" {
#include "cpu/perfect6502.h"
#include "cpu/types.h"
#include "cpu/netlist_sim.h"
//#include "cpu/netlist_6502.h"
}

#include <set>
#include <unordered_map>

/* Output memory file:
 31-16: address
15:8: data
    3: Vector pull
    2: Memory Lock
    1: sync
 0: read/write
*/

struct ExtSignal {
    size_t count, delay;
};

enum { ExtReady, ExtSo, ExtNmi, ExtReset, ExtIrq };
ExtSignal signals[5];

void record_bus( void *cpu_state, std::ostream &logger, const char *comment ) {
    logger<<"1_";

    char buffer[5];
    snprintf(buffer, 5, "%04x", readAddressBus(cpu_state) );
    logger<<buffer<<"_";

    snprintf(buffer, 5, "%02x", readDataBus(cpu_state) );
    logger<<buffer<<"_";

    if( readRW(cpu_state) )
        logger<<"01";
    else
        logger<<"00";

    if( comment ) {
        logger<<"\t\t// "<<comment;
    }

    logger<<"\n";
}

struct CycleTask {
    std::set<nodenum_t> raise, lower;
};

std::unordered_map<size_t, CycleTask> cycleTasks;
size_t cycle_num = 0;

void schedule_signal_change(size_t delay, size_t count, nodenum_t node, bool polarity) {
    size_t start_cycle = cycle_num + delay;
    size_t end_cycle = start_cycle + count;
    std::cerr<<"Scheduling IO: "<<cycle_num<<" node "<<node<<" polarity "<<polarity<<" start "<<start_cycle<<" end "<<end_cycle<<"\n";
    cycleTasks.try_emplace(polarity ? start_cycle : end_cycle).first->second.raise.insert(node);
    cycleTasks.try_emplace(polarity ? end_cycle : start_cycle).first->second.lower.insert(node);
}

int main(int argc, char *argv[]) {
    if( argc<2 ) {
        std::cerr<<"Usage: "<<argv[0]<<" test_program.mem\n";

        return 2;
    }
    ReadMem<8> test_program_mem(argv[1]);
    std::unordered_map<size_t, std::string> commented_lines;

    static constexpr nodenum_t
            RDY = 89,
            RES = 159,
            IRQ = 103,
            NMI = 1297,
            SO = 1672;

    while( test_program_mem.read_line() ) {
        memory[test_program_mem.address()] = test_program_mem[0];

        if( ! test_program_mem.comment().empty() ) {
            commented_lines.emplace( test_program_mem.address(), test_program_mem.comment() );
        }
    }

    void *state = initAndResetChip();

    setNode(state, SO, true);
    setNode(state, NMI, true);

    bool clk = false;
    bool recording = false;
    bool done = false;
    while( !done ) {
        step(state);

        if( clk && readAddressBus(state)==0xfffc )
            recording = true;

        if( recording && clk ) {
            auto comment = commented_lines.find(readAddressBus(state));
            record_bus(state, std::cout, comment!=commented_lines.end() ? comment->second.c_str() : nullptr);

            auto tasks_iter = cycleTasks.find(cycle_num);
            if( tasks_iter != cycleTasks.end() ) {
                for( auto node : tasks_iter->second.raise ) {
                    std::cerr<<"Raise "<<node<<"\n";
                    setNode(state, node, true);
                }
                for( auto node : tasks_iter->second.lower ) {
                    std::cerr<<"Lower "<<node<<"\n";
                    setNode(state, node, false);
                }

                cycleTasks.erase(tasks_iter);
            }

            if( (readAddressBus(state) & 0xff00) == 0x0200 && readRW(state)==0 ) {
                switch( readAddressBus(state)&0xff ) {
                case 0x00: done = true; break;
                case 0x80: signals[ExtReady].count = readDataBus(state); break;
                case 0x81: schedule_signal_change(readDataBus(state), signals[ExtReady].count, RDY, false); break;
                case 0x82: signals[ExtSo].count = readDataBus(state); break;
                case 0x83: schedule_signal_change(readDataBus(state), signals[ExtSo].count, SO, false); break;
                case 0xfa: signals[ExtNmi].count = readDataBus(state); break;
                case 0xfb: schedule_signal_change(readDataBus(state), signals[ExtNmi].count, NMI, false); break;
                case 0xfc: signals[ExtReset].count = readDataBus(state); break;
                case 0xfd: schedule_signal_change(readDataBus(state), signals[ExtReset].count, RES, false); break;
                case 0xfe: signals[ExtIrq].count = readDataBus(state); break;
                case 0xff: schedule_signal_change(readDataBus(state), signals[ExtIrq].count, IRQ, false); break;
                }
            }

            cycle_num++;
        }

        clk = !clk;
    }
}

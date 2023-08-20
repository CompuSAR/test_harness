#include "readmem.h"

extern "C" {
#include "cpu/perfect6502.h"
#include "cpu/types.h"
#include "cpu/netlist_sim.h"
//#include "cpu/netlist_6502.h"
}

#include <fstream>
#include <functional>
#include <unordered_map>
#include <vector>

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

void record_bus( void *cpu_state, std::ostream &logger ) {
    logger<<"1_";

    char buffer[5];
    snprintf(buffer, 5, "%04x", readAddressBus(cpu_state) );
    logger<<buffer<<"_";

    snprintf(buffer, 5, "%02x", readDataBus(cpu_state) );
    logger<<buffer<<"_";

    if( readRW(cpu_state) )
        logger<<"01\n";
    else
        logger<<"00\n";
}

std::unordered_map<size_t, std::vector< std::function<void()> >> callbacks;
size_t cycle_num = 0;

void add_callback(size_t cycle, std::function<void()> cb) {
    auto inserter = callbacks.emplace( cycle, std::vector< std::function<void()> >() );
    inserter.first->second.push_back( cb );
}

void schedule_signal_change(void *cpu_state, size_t delay, size_t count, nodenum_t node) {
    add_callback(cycle_num+delay,
            [cpu_state, count, node]() {
                setNode( cpu_state, node, false );
                add_callback( cycle_num+count, [node, cpu_state]() {
                            setNode( cpu_state, node, true );
                        });
            });
}

int main(int argc, char *argv[]) {
    ReadMem<8> test_program_mem(argv[1]);
    std::unordered_map<size_t, std::string> commented_lines;
    std::ofstream recorded_bus(argv[2]);

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

    bool clk = false;
    bool recording = false;
    bool done = false;
    while( !done ) {
        step(state);

        std::cout<<cycle<<"\n";
        if( clk && readAddressBus(state)==0xfffc )
            recording = true;

        if( recording && clk ) {
            record_bus(state, recorded_bus);

            auto callback_vect = callbacks.find(cycle_num);
            if( callback_vect != callbacks.end() ) {
                for( auto &callback : callback_vect->second ) {
                    callback();
                }
                callbacks.erase(callback_vect);
            }

            if( (readAddressBus(state) & 0xff00) == 0x0200 && readRW(state)==0 ) {
                switch( readAddressBus(state)&0xff ) {
                case 0x00: done = true; break;
                case 0x80: signals[ExtReady].count = readDataBus(state); break;
                case 0x81: schedule_signal_change(state, readDataBus(state), signals[ExtReady].count, RDY); break;
                case 0x82: signals[ExtSo].count = readDataBus(state); break;
                case 0x83: schedule_signal_change(state, readDataBus(state), signals[ExtSo].count, SO); break;
                case 0xfa: signals[ExtNmi].count = readDataBus(state); break;
                case 0xfb: schedule_signal_change(state, readDataBus(state), signals[ExtNmi].count, NMI); break;
                case 0xfc: signals[ExtReset].count = readDataBus(state); break;
                case 0xfd: schedule_signal_change(state, readDataBus(state), signals[ExtReset].count, RES); break;
                case 0xfe: signals[ExtIrq].count = readDataBus(state); break;
                case 0xff: schedule_signal_change(state, readDataBus(state), signals[ExtIrq].count, IRQ); break;
                }
            }

            cycle_num++;
        }

        clk = !clk;
    }
}

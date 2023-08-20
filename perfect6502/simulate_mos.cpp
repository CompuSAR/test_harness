#include "readmem.h"

int main(int argc, char *argv[]) {
    ReadMem<8> test_program_mem(argv[1]);

    while( test_program_mem.read_line() ) {
    }
}

#include "readmem.h"

#include <regex>
#include <system_error>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static std::span<const char> open_and_map(std::filesystem::path path) {
    int fd = open(path.c_str(), O_RDONLY);
    if( fd==-1 )
        throw std::system_error( std::error_code(errno, std::system_category()), "Failed to open MEM file" );

    struct stat stat;
    if( fstat(fd, &stat)==-1 )
        throw std::system_error( std::error_code(errno, std::system_category()), "Failed to stat MEM file" );

    const char *mem = (const char *)mmap(nullptr, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if( mem==MAP_FAILED )
        throw std::system_error( std::error_code(errno, std::system_category()), "Mapping MEM file to memory failed" );

    close(fd);

    return std::span(mem, stat.st_size);
}

static uint8_t parse_hex_digit(char ch) {
    switch( ch ) {
#define CASE(a) case #a[0]: return 0x##a
        CASE(0);
        CASE(1);
        CASE(2);
        CASE(3);
        CASE(4);
        CASE(5);
        CASE(6);
        CASE(7);
        CASE(8);
        CASE(9);
        CASE(a);
        CASE(b);
        CASE(c);
        CASE(d);
        CASE(e);
        CASE(f);
        CASE(A);
        CASE(B);
        CASE(C);
        CASE(D);
        CASE(E);
        CASE(F);
#undef CASE
    default:
        throw ParseError();
    }
}

ReadMemSupport::ReadMemSupport(std::filesystem::path memfile) :
    _data(open_and_map(memfile)),
    _rest_of_file(_data)
{
}

ReadMemSupport::~ReadMemSupport() {
    if( ! _data.empty() ) {
        munmap(const_cast<char *>( _data.data() ), _data.size());

        _data = std::span<const char>();
    }
}

bool ReadMemSupport::nextDatum(DataContainer &datum, size_t &address, std::string &comment) {
    comment.clear();
    if( _line.empty() ) {
        for( size_t i=0; i<_rest_of_file.size(); ++i ) {
            if( _rest_of_file[i]=='\n' ) {
                _line = _rest_of_file.first(i);
                //_rest_of_file = _rest_of_file.last(_rest_of_file.size() - i - 1);
                _rest_of_file = _rest_of_file.subspan(i+1);

                _line_number++;

                break;
            }
        }

        if( _line.empty() ) {
            _done = true;
            return false;
        }

        // Parse the comments first
        static std::regex comment_parser( ".*//(.*)" );
        std::cmatch re_result;
        if( std::regex_match( &*_line.begin(), &*_line.begin()+_line.size(), re_result, comment_parser ) ) {
            comment = std::string( re_result.str(1) );
            _line = _line.first( re_result.position(1) - 2 );
        }
    }

    skip_ws();

    while( _line[0]=='@' ) {
        _line = _line.subspan(1);

        _address = parse_hex_number();
    }

    address = _address++;

    if( _line.empty() )
        return false;

    datum = parse_hex_number();

    return true;
}

void ReadMemSupport::skip_ws() {
    while( !_line.empty() && (_line[0]==' ' || _line[0]=='\t') )
        _line = _line.subspan(1);
}

ReadMemSupport::DataContainer ReadMemSupport::parse_hex_number() {
    DataContainer res = 0;

    bool valid = false;
    while( !_line.empty() && _line[0]!=' ' && _line[0]!='\t' ) {
        if( _line[0]=='_' ) {
            _line = _line.subspan(1);
            continue;
        }

        res <<= 4;
        res |= parse_hex_digit(_line[0]);
        valid = true;
        _line = _line.subspan(1);
    }

    if( !valid )
        throw ParseError();

    skip_ws();

    return res;
}

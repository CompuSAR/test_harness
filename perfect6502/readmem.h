#pragma once

#include <filesystem>
#include <span>

#include <iostream>

#include <stdlib.h>

class ParseError : public std::exception {
};

class ReadMemSupport final {
public:
    static constexpr size_t MaxDataSize = 64;
    using DataContainer = uint64_t;

private:
    std::span<const char>               _data, _rest_of_file, _line;
    size_t                              _line_number = 0;
    DataContainer                       _address = 0;
    bool                                _done = false;

public:
    explicit ReadMemSupport() = default;
    explicit ReadMemSupport(std::filesystem::path memfile);
    ~ReadMemSupport();

    ReadMemSupport(const ReadMemSupport &that) = delete;
    ReadMemSupport &operator=(const ReadMemSupport &that) = delete;
    ReadMemSupport(ReadMemSupport &&that) = delete;
    ReadMemSupport &operator=(ReadMemSupport &&that) = delete;

    /* implicit */ operator const std::span<const char> &() const {
        return _data;
    }

    size_t lineNumber() const {
        return _line_number;
    }

    bool nextDatum(DataContainer &datum, size_t &address, std::string &comment);
    bool done() const {
        return _done;
    }

private:
    void skip_ws();
    DataContainer parse_hex_number();
};

template<size_t... BitFields>
class ReadMem {
    ReadMemSupport _base; 
    std::array<ReadMemSupport::DataContainer, sizeof...(BitFields)> _fields;
    size_t _address;
    std::string _comment;

    template<typename... Fields>
    static constexpr size_t total_bit_size(size_t first_field, Fields... fields) {
        if constexpr(sizeof...(Fields)==0)
            return first_field;
        else {
            return first_field+total_bit_size(fields...);
        }
    }

    static_assert( total_bit_size(BitFields...) <= ReadMemSupport::MaxDataSize, "Total field must fit in 64 bit int"  );
public:
    explicit ReadMem(std::filesystem::path memfile) : _base(memfile) {}

    bool read_line() {
        ReadMemSupport::DataContainer datum;

        while( ! _base.done() ) {
            if( _base.nextDatum(datum, _address, _comment) ) {
                split_result(0, datum, BitFields...);
                return true;
            }
        }

        return false;
    }

    size_t address() const {
        return _address;
    }
    std::string comment() const {
        return _comment;
    }
    ReadMemSupport::DataContainer operator[](size_t index) const {
        return _fields.at(index);
    }

    size_t lineNumber() const {
        return _base.lineNumber();
    }

private:
    template<typename... Lengths>
    void split_result(size_t index, ReadMemSupport::DataContainer data, size_t firstLength, Lengths... lengths) {
        _fields[index] = data & ( (1UL<<firstLength) - 1 );
        data >>= firstLength;

        if constexpr( sizeof...(Lengths) != 0 )
            split_result(index+1, data, lengths...);
    }
};

inline std::ostream &operator<<(std::ostream &out, std::span<const char> span) {
    for( char c : span ) {
        out<<c;
    }

    return out;
}

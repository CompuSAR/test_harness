import re


def array_to_number(data: list[int]) -> int:
    result = 0
    for datum in data:
        result *= 16
        result += datum

    return result

class MemFile:
    """
    Reads and parses a Xilinx memory file.

    Parameters:
    file_handle (input file):   file to read the data from.
    num_digits (int):           How many digits to expect in each line
    """
    _parser_re = re.compile(
            r"""
            ^
            (?P<empty_spaces> \s+) |
            (?P<comment> //.*) |
            (?P<multiline_comment> /[*]) |
            (@ (?P<address> [0-9a-fA-F]+)) |
            (?P<data> [0-9a-fA-F] [0-9a-fA-F_]*)
            """,
            re.VERBOSE)
    _comment_re = re.compile(
            r"""
            ^
            (?P<body> ([^*] | [*][^/])* )
            (?P<end> [*]/)?
            """,
            re.VERBOSE)

    def __init__(self, file_handle, *, name: str = "", num_digits: int):
        self._file_handle = file_handle
        self._num_digits = num_digits

    def __del__(self):
        self._file_handle.close()

    def __iter__(self):
        self.line_num = 0
        address = 0
        in_comment = False

        for line in self._file_handle:
            self.line_num += 1
            self.col_num = 1

            while line:
                if in_comment:
                    regexp = self._comment_re
                else:
                    regexp = self._parser_re

                parsed = regexp.match(line)
                if parsed is None:
                    raise RuntimeError(f'Failed to parse memory file {str(self._file_handle)} at {self.line_num}:{self.col_num}')

                line = line[ parsed.span()[1]: ]
                self.col_num += parsed.span()[1]-1

                if in_comment:
                    if parsed['end'] is not None:
                        in_comment = False

                    continue

                if parsed['multiline_comment'] is not None:
                    in_comment = True

                elif parsed['address'] is not None:
                    address = int(parsed['address'], 16)

                elif parsed['data'] is not None:
                    data: list[int] = []

                    str_data: str = parsed['data']

                    for i in str_data:
                        if i == '_':
                            continue

                        data.append( int(i, 16) )

                    if len(data) != self._num_digits:
                        raise RuntimeError(
                                f'Wrong line length at {self._file_handle.name}:{self.line_num}:{self.col_num}: Expected {self._num_digits} digits, parsed {len(data)}')

                    yield (address, data)

                    address += 1

        del self.line_num
        del self.col_num

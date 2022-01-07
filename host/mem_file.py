import re


class MemFile:
    _parser_re = re.compile(
            r"""
            ^
            (?P<empty_spaces> \s+) |
            (?P<comment> //.*) |
            (@ (?P<address> [0-9a-fA-F]+)) |
            (?P<data> [0-9a-fA-F] [0-9a-fA-F_]*)
            """,
            re.VERBOSE)

    def __init__(self, file_handle, *, name: str = ""):
        self._file_handle = file_handle

    def __del__(self):
        self._file_handle.close()

    def __iter__(self):
        line_num = 0
        address = 0

        for line in self._file_handle:
            line_num += 1
            col_num = 1

            while line:
                parsed = self._parser_re.match(line)
                if parsed is None:
                    raise RuntimeError(f'Failed to parse memory file {self._file_handle.name} at {line_num}:{col_num}')

                line = line[ parsed.span()[1]: ]
                col_num += parsed.span()[1]-1

                if parsed['address'] is not None:
                    address = int(parsed['address'], 16)

                elif parsed['data'] is not None:
                    data: list[int] = []

                    str_data: str = parsed['data']

                    for i in str_data[::-1]:
                        if i == '_':
                            continue

                        data.append( int(i, 16) )

                    yield (address, data)

                    address += 1

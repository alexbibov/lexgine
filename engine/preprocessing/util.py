import re
from typing import Iterable, Tuple, Any, List, Optional
from pathlib import Path
from functools import reduce

import global_names
import common_types


def signal_first(series: Iterable) -> Iterable[Tuple[bool, Any]]:
    it = iter(series)
    try:
        rv = next(it)
        yield True, rv
        for rv in it:
            yield False, rv
    except StopIteration:
        return True, None


def signal_last(series: Iterable) -> Iterable[Tuple[bool, Any]]:
    it = iter(series)
    try:
        rv = next(it)
        for v in it:
            yield False, rv
            rv = v
        yield True, rv
    except StopIteration:
        return True, None


def is_hpp_path_ignored(hpp_path: Path,
                        preprocessor_ignored_hpp_paths: List[Path]):
    try:
        engine_subdirectory_index = hpp_path.parts.index("engine")
    except ValueError:
        return False

    hpp_path_engine_relative_parts = hpp_path.parts[engine_subdirectory_index:]
    engine_relative_hpp_path = Path(
        reduce(lambda x, y: f"{x}/{y}", hpp_path_engine_relative_parts[1:],
               hpp_path_engine_relative_parts[0]))

    if engine_relative_hpp_path in preprocessor_ignored_hpp_paths:
        return True

    def is_subdirectory(tested_path: Path, related_path: Path) -> bool:
        tested_path_parts = tested_path.parts
        related_path_parts = related_path.parts
        if len(tested_path_parts) < len(related_path_parts):
            return False
        for idx, p in enumerate(related_path_parts):
            if p != tested_path_parts[idx]:
                return False
        return True

    for ignored_path in preprocessor_ignored_hpp_paths:
        if is_subdirectory(engine_relative_hpp_path, ignored_path):
            return True

    return False


def locate_closing_token(source: str, opening_token_offset: int) -> int:
    supported_opening_tokens = reduce(lambda x, y: x + y, [str(s.value[0]) for s in common_types.ScopeType], '')
    closing_tokens = reduce(lambda x, y: x + y, [str(s.value[1]) for s in common_types.ScopeType], '')

    if source[opening_token_offset] not in supported_opening_tokens:
        ValueError(f"'{source[opening_token_offset]}' is not supported opening token")

    opening_token = source[opening_token_offset]
    closing_token = closing_tokens[supported_opening_tokens.find(opening_token)]
    counter = 1
    i: int = 0
    for i in range(opening_token_offset + 1, len(source)):
        if source[i] == opening_token:
            counter += 1
        elif source[i] == closing_token:
            counter -= 1
        if counter == 0:
            break
    return i


def extract_scope(source: str, offset: int, scope_type: common_types.ScopeType) -> Tuple[int, int]:
    opening_brace_idx = source.find(scope_type.value[0], offset)
    if opening_brace_idx == -1:
        return -1, -1
    return opening_brace_idx, locate_closing_token(source, opening_brace_idx)


# checks if name_token1 is sub-name of name_token2
# for example is_subname("core::my_type", "lexgine::core::my_type") returns True
def is_subname(name_token1: str, name_token2: str) -> bool:
    name_token1_parts = name_token1.split('::')
    name_token2_parts = name_token2.split('::')

    if len(name_token1_parts) > len(name_token2_parts):
        return False

    for idx, token in enumerate(reversed(name_token1_parts)):
        if name_token2_parts[-1 - idx] != token:
            return False

    return True


def count_lines_till_index(source: str, offset: int, index: int) -> int:
    return len(source[offset:index + 1].splitlines())


def get_line_offset(source: str, zero_based_line_number: int) -> int:
    return reduce(lambda x, y: x + len(y) + 1, source.splitlines()[:zero_based_line_number], 0)


def find_first_of(source: str, chars: str, start: Optional[int] = None, end: Optional[int] = None) -> int:
    return min([e for e in [source.find(c, start, end) for c in chars] if e != -1])


def convert_camel_case_to_snake_case(token: str) -> str:
    camel_case_re = re.compile(r'(?<!^)(?<!D3)(?=[A-Z])')
    return re.sub(camel_case_re, '_', token).lower()


def convert_snake_case_to_camel_case(token: str) -> str:
    token = token.lower()
    replacements = []
    for i in range(len(token)):
        if token[i] == '_' and token[i + 1].isalpha():
            replacements.append((i, token[i + 1].upper()))

    rv = ""
    li = 0
    for i, v in replacements:
        rv += token[li:i] + v
        li = i + 2
    if li < len(token):
        rv += token[li:]
    return rv


def fetch_files_from_directory(folder: Path) -> List[Path]:
    rv = []
    for e in folder.iterdir():
        if e.is_file():
            rv.append(e)
        elif e.is_dir():
            rv += fetch_files_from_directory(e)
    return rv


def is_source_file(file: Path) -> bool:
    return file.suffix in (".cpp", ".c")


def is_header_file(file: Path) -> bool:
    return file.suffix in (".hpp", ".h", ".inl")
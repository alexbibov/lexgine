import re
from typing import Optional, List, Tuple, Set
from pathlib import Path
from functools import reduce

import global_names
import common_types
import util


def extract_engine_path(supplied_include_path: Path) -> Optional[Path]:
    path_parts = list(supplied_include_path.parts)
    try:
        engine_folder_part_idx = path_parts.index("engine")

        engine_path_prefix_parts = path_parts[:engine_folder_part_idx]
        engine_path = ""
        for p in engine_path_prefix_parts:
            engine_path += f"{p}/"
        return Path(engine_path)

    except ValueError:
        return None


def extract_header_relative_path(supplied_include_path: Path, engine_path: Path) -> Optional[Path]:
    return supplied_include_path.relative_to(engine_path)


def create_api_path(source_path: Path) -> Optional[Path]:
    path_parts = list(source_path.parts[:-1])
    try:
        path_parts = path_parts[path_parts.index("engine"):]
        path_parts[0] = "api"
        fixed_path = reduce(lambda x, y: x + "/" + y, path_parts[1:], path_parts[0])
        fixed_path = Path(fixed_path)
        return fixed_path
    except ValueError:
        return None


def is_engine_path(source_path: Path) -> bool:
    path_parts = source_path.parts
    return True if len(path_parts) > 1 and path_parts[0] == "engine" else False


def convert_engine_path_to_api_path(source_path: Path) -> Optional[Path]:
    if is_engine_path(source_path):
        path_parts = list(source_path.parts)
        path_parts[0] = "api"
        converted_path = reduce(lambda x, y: f"{x}/{y}", path_parts[1:],
                                path_parts[0])
        return Path(converted_path)
    return None


def search_for_lexgine_namespace(source: str, offset: int) -> Optional[common_types.LexgineNamespace]:
    search_offset = offset
    lexgine_namespace_start_idx = -1
    while lexgine_namespace_start_idx == -1 and search_offset < len(source):
        idx = source.find("namespace", search_offset)
        if idx == -1:
            break

        lexgine_namespace_start_idx = idx + len("namespace")
        subsource = source[lexgine_namespace_start_idx:].lstrip()
        if not subsource.startswith(global_names.LEXGINE_NAMESPACE_PREFIX):
            search_offset = lexgine_namespace_start_idx
            lexgine_namespace_start_idx = -1

    if lexgine_namespace_start_idx == -1:
        return None
    else:
        lexgine_namespace_begin_idx, lexgine_namespace_end_idx = util.extract_scope(
            source, lexgine_namespace_start_idx, common_types.ScopeType.BRACES
        )
        lexgine_namespace_definition_token = source[lexgine_namespace_start_idx:lexgine_namespace_begin_idx].strip()

        lexgine_namespace_begin_line = util.count_lines_till_index(source, 0, lexgine_namespace_begin_idx)
        lexgine_namespace_end_line = util.count_lines_till_index(source, 0, lexgine_namespace_end_idx)

        return common_types.LexgineNamespace(
            lexgine_namespace_definition_token,
            lexgine_namespace_begin_line,
            lexgine_namespace_end_line,
            lexgine_namespace_begin_idx,
            lexgine_namespace_end_idx
        )


def get_lexgine_namespaces(source: str) -> List[common_types.LexgineNamespace]:
    rv = []
    new_lexgine_namespace = search_for_lexgine_namespace(source, 0)
    while new_lexgine_namespace is not None:
        rv.append(new_lexgine_namespace)
        new_lexgine_namespace = search_for_lexgine_namespace(source, new_lexgine_namespace.end_index + 1)
    return rv


def get_lexgine_namespace_for_location(lexgine_namespaces: List[common_types.LexgineNamespace],
                                       line: int) -> Optional[common_types.LexgineNamespace]:
    for namespace in lexgine_namespaces:
        if namespace.begin_line_number <= line <= namespace.end_line_number:
            return namespace
    return None


def list_raw_lexgine_export_references(
        source: str,
        lexgine_namespaces: List[common_types.LexgineNamespace]
) -> Tuple[Set[common_types.LexgineTypeReference], str]:
    token_function_re = re.compile(r"[\w\d\s:&*_<>]+\s+(?P<function_name>\w+)\s*\([\w\d\s&:*_,<>]*\)",
                                   flags=re.MULTILINE)
    token_flags_re = re.compile(r"BEGIN_FLAGS_DECLARATION\s*\(\s*(?P<flags_name>\w+)\s*\)", flags=re.MULTILINE)

    supported_token_definition_strings = [str(s.value) for s in common_types.TokenType if isinstance(s.value, str)]
    rv = set()

    offset = 0
    idx = source.find(global_names.CPP_INTERFACE_LOOKUP_KEYWORD, offset)

    while idx > 0:
        export_reference_definition_line_idx = util.count_lines_till_index(source, 0, idx) - 1
        type_definition_line_offset = util.get_line_offset(source, export_reference_definition_line_idx)

        token_type = common_types.TokenType.UNKNOWN
        for token_string in supported_token_definition_strings:
            if source.find(token_string, type_definition_line_offset, idx) > 0:
                token_type = common_types.TokenType(token_string)
                break

        exported_type_definition_begin_offset = idx + len(global_names.CPP_INTERFACE_LOOKUP_KEYWORD)
        stripped_source = source[exported_type_definition_begin_offset:].lstrip(' \t\r\n')

        dependency_list_offset = stripped_source.find(global_names.DEPENDENCY_KEYWORD, 0)
        dependency_list_token_length = 0
        dependency_list = ()
        if dependency_list_offset == 0:
            dependency_list_begin, dependency_list_end = util.extract_scope(
                stripped_source, dependency_list_offset, common_types.ScopeType.PARENTHESES)
            dependency_list_token_length = dependency_list_end - dependency_list_offset + 1
            dependency_list = tuple([
                e.strip()
                for e in stripped_source[dependency_list_begin + 1:dependency_list_end].split(sep=',')
            ])
            stripped_source = stripped_source[dependency_list_token_length:].lstrip(' \t\r\n')

        if token_type != common_types.TokenType.UNKNOWN:
            token_name = stripped_source
            token_name = token_name[:util.find_first_of(token_name, '{; \n\r')]
        else:
            m = token_function_re.match(stripped_source)
            if m is not None:
                # check if current export is a function
                token_name = m["function_name"]
                token_type = token_type.FUNCTION
            else:
                m = token_flags_re.match(stripped_source)
                if m is not None:
                    token_name = m["flags_name"]
                    token_type = token_type.FLAGS

        if token_type == common_types.TokenType.UNKNOWN:
            raise AssertionError(
                f'Unable to determine type of the token at line {export_reference_definition_line_idx + 1}, '
                f'"{source[idx:idx + 100]}"...'
            )

        parent_namespace = get_lexgine_namespace_for_location(lexgine_namespaces, export_reference_definition_line_idx)
        if parent_namespace is not None:
            rv.add(
                common_types.LexgineTypeReference(
                    name=token_name,
                    line=export_reference_definition_line_idx + 1,
                    token_type=token_type,
                    parent_namespace=parent_namespace,
                    dependency_list=dependency_list
                )
            )
        else:
            print(
                f"Found Lexgine type export {token_name} outside of any Lexgine namespace. "
                f"This type export will be ignored."
            )

        source = source[:idx] + source[idx + len(global_names.CPP_INTERFACE_LOOKUP_KEYWORD)
                                       + dependency_list_token_length + 1:]

        offset = idx
        idx = source.find(global_names.CPP_INTERFACE_LOOKUP_KEYWORD, offset)

    return rv, source


import argparse
import datetime
import functools
import json
import re
import string
from string import Template
from collections import namedtuple
from pathlib import Path
from CppHeaderParser import CppHeader, CppParseError, CppClass, CppMethod, CppVariable
from typing import List, Optional, Tuple, Dict, Set, Iterable, Any, Union
from enum import Enum
from functools import reduce


class TokenType(Enum):
    ENUM = "enum"
    CLASS = "class"
    STRUCT = "struct"
    UNION = "union"
    FUNCTION = 4
    FLAGS = 5
    UNKNOWN = 6


class ScopeType(Enum):
    BRACES = "{}"
    PARENTHESES = "()"
    SQUARE_BRACKETS = "[]"
    ANGULAR_BRACKETS = "<>"


class LexgineFlags:
    _flag_entry_re = re.compile(r"FLAG\s*\(\s*(?P<flag_name>\w+)\s*,\s*(?P<flag_value>\w+)\s*\)")

    def __init__(self, source: str, declaration_begin_offset_idx: int, name: str):
        source = source[declaration_begin_offset_idx:].lstrip(' \n\t\r')
        _flags_declaration_begin_re = re.compile(r"BEGIN_FLAGS_DECLARATION\s*\(\s*" + name + r"\s*\)")
        _flag_declaration_end_re = re.compile(r"END_FLAGS_DECLARATION\s*\(\s*" + name + r"\s*\)")

        declaration_begin = _flags_declaration_begin_re.search(source)
        declaration_end = _flag_declaration_end_re.search(source)

        if declaration_begin is None or declaration_end is None:
            raise RuntimeError(f"Unable to locate declaration of flags '{name}' in provided source string")

        self._declaration_string = source[declaration_begin.start():declaration_end.end()]
        self._declaration_flags: Dict[str, str] = {}
        self._name = name

        declaration_flag = LexgineFlags._flag_entry_re.search(self._declaration_string)
        while declaration_flag is not None:
            self._declaration_flags[declaration_flag["flag_name"]] = declaration_flag["flag_value"]
            declaration_flag = LexgineFlags._flag_entry_re.search(self._declaration_string, declaration_flag.end())

    @property
    def name(self) -> str:
        return self._name

    @property
    def declaration_string(self) -> str:
        return self._declaration_string

    def __iter__(self):
        return iter(self._declaration_flags.items())


LexgineNamespace = namedtuple("LexgineNamespace", ["definition", "begin_line_number", "end_line_number", "begin_index", "end_index"])
LexgineTypeReference = namedtuple("LexgineTypeReference", ["name", "line", "token_type", "parent_namespace"])


class LexgineDllExportInterface(namedtuple("LexgineDllExportInterface", ["source"])):
    def write(self, path: Path, interface_name: str):
        if self.source is not None:
            with open((path / f"{interface_name}.cpp").as_posix(), 'w') as f:
                f.write(self.source)


class LexgineRuntimeLinkInterface(namedtuple("LexgineRuntimeLinkInterface", ["source", "header"])):
    def write(self, path: Path, interface_name: str):
        if self.header is not None:
            with open((path / f"{interface_name}.h").as_posix(), 'w') as f:
                f.write(self.header)

        if self.source is not None:
            with open((path / f"{interface_name}.cpp").as_posix(), 'w') as f:
                f.write(self.source)


PreprocessorHeaderBillet = namedtuple("PreprocessorHeaderBillet", ["exporting_header_path", "exported_types_set", "preprocessor_ready_header_source"])


CPP_INTERFACE_LOOKUP_KEYWORD = "LEXGINE_CPP_API"
LUA_INTERFACE_LOOKUP_KEYWORD = "LEXGINE_LUA_API"
LEXGINE_API_KEYWORD = "LEXGINE_API"

argument_parser = argparse.ArgumentParser(description="Lexgine DLL interface generator")
argument_parser.add_argument("--headers", required=True, help="List of paths to the C++ headers to be parsed",
                             dest="headers")
argument_parser.add_argument("--config", required=True, help="Path to configuration JSON", dest="config")
argument_parser.add_argument("--output", required=True, help="Output directory where to write the generated interfaces",
                             dest="output", type=Path)
arguments = argument_parser.parse_args()

with open(arguments.config, 'r') as f:
    configuration = json.load(f)


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


def is_hpp_path_ignored(hpp_path: Path, preprocessor_ignored_hpp_paths: List[Path]):
    try:
        engine_subdirectory_index = hpp_path.parts.index("engine")
    except ValueError:
        return False

    hpp_path_engine_relative_parts = hpp_path.parts[engine_subdirectory_index:]
    engine_relative_hpp_path = Path(reduce(lambda x, y: x + '/' + y, hpp_path_engine_relative_parts[1:], hpp_path_engine_relative_parts[0]))

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
    supported_opening_tokens = reduce(lambda x, y: x + y, [str(s.value[0]) for s in ScopeType], '')
    closing_tokens = reduce(lambda x, y: x + y, [str(s.value[1]) for s in ScopeType], '')

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


def extract_scope(source: str, offset: int, scope_type: ScopeType) -> Tuple[int, int]:
    opening_brace_idx = source.find(scope_type.value[0], offset)
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
    return len(source[offset:index+1].splitlines())


def get_line_offset(source: str, zero_based_line_number: int) -> int:
    return functools.reduce(lambda x, y: x + len(y) + 1, source.splitlines()[:zero_based_line_number], 0)


def search_for_lexgine_namespace(source: str, offset: int) -> Optional[LexgineNamespace]:
    search_offset = offset
    lexgine_namespace_start_idx = -1
    while lexgine_namespace_start_idx == -1 and search_offset < len(source):
        idx = source.find("namespace", search_offset)
        if idx == -1:
            break

        lexgine_namespace_start_idx = idx + len("namespace")
        subsource = source[lexgine_namespace_start_idx:].lstrip()
        if not subsource.startswith("lexgine"):
            search_offset = lexgine_namespace_start_idx
            lexgine_namespace_start_idx = -1

    if lexgine_namespace_start_idx == -1:
        return None
    else:
        lexgine_namespace_begin_idx, lexgine_namespace_end_idx = extract_scope(source, lexgine_namespace_start_idx, ScopeType.BRACES)
        lexgine_namespace_definition_token = source[lexgine_namespace_start_idx:lexgine_namespace_begin_idx].strip()

        lexgine_namespace_begin_line = count_lines_till_index(source, 0, lexgine_namespace_begin_idx)
        lexgine_namespace_end_line = count_lines_till_index(source, 0, lexgine_namespace_end_idx)

        return LexgineNamespace(lexgine_namespace_definition_token,
                                lexgine_namespace_begin_line, lexgine_namespace_end_line,
                                lexgine_namespace_begin_idx, lexgine_namespace_end_idx)


def get_lexgine_namespaces(source: str) -> List[LexgineNamespace]:
    rv = []
    new_lexgine_namespace = search_for_lexgine_namespace(source, 0)
    while new_lexgine_namespace is not None:
        rv.append(new_lexgine_namespace)
        new_lexgine_namespace = search_for_lexgine_namespace(source, new_lexgine_namespace.end_index + 1)
    return rv


def get_lexgine_namespace_for_location(lexgine_namespaces: List[LexgineNamespace], line: int) -> Optional[LexgineNamespace]:
    for namespace in lexgine_namespaces:
        if namespace.begin_line_number <= line <= namespace.end_line_number:
            return namespace
    return None


def find_first_of(source: str, chars: str, start: Optional[int] = None, end: Optional[int] = None) -> int:
    return min([e for e in [source.find(c, start, end) for c in chars] if e != -1])


def list_raw_lexgine_export_references(source: str, lexgine_namespaces: List[LexgineNamespace]) -> Tuple[Set[LexgineTypeReference], str]:
    token_function_re = re.compile(r"[\w:]+\s+(?P<function_name>\w+)\s*\([\w\d\s&:\*_,]*\)", flags=re.MULTILINE)
    token_flags_re = re.compile(r"BEGIN_FLAGS_DECLARATION\s*\(\s*(?P<flags_name>\w+)\s*\)", flags=re.MULTILINE)

    supported_token_definition_strings = [str(s.value) for s in TokenType if isinstance(s.value, str)]
    rv = set()

    offset = 0
    idx = source.find(CPP_INTERFACE_LOOKUP_KEYWORD, offset)

    while idx > 0:
        export_reference_definition_line_idx = count_lines_till_index(source, 0, idx) - 1
        type_definition_line_offset = get_line_offset(source, export_reference_definition_line_idx)

        token_type = TokenType.UNKNOWN
        for token_string in supported_token_definition_strings:
            if source.find(token_string, type_definition_line_offset, idx) > 0:
                token_type = TokenType(token_string)
                break

        exported_type_definition_begin_offset = idx + len(CPP_INTERFACE_LOOKUP_KEYWORD)
        stripped_source = source[exported_type_definition_begin_offset:].lstrip(' \t\r\n')
        if token_type != TokenType.UNKNOWN:
            token_name = source[exported_type_definition_begin_offset:].lstrip(' \t\r\n')
            token_name = token_name[:find_first_of(token_name, '{; \n\r')]
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

        if token_type == TokenType.UNKNOWN:
            raise AssertionError(f'Unable to determine type of the token at line {export_reference_definition_line_idx + 1}, '
                                 f'"{source[idx:idx + 100]}"...')

        parent_namespace = get_lexgine_namespace_for_location(lexgine_namespaces, export_reference_definition_line_idx)
        if parent_namespace is not None:
            rv.add(LexgineTypeReference(name=token_name, line=export_reference_definition_line_idx + 1,
                                        token_type=token_type, parent_namespace=parent_namespace))
        else:
            print(f"Found Lexgine type export {token_name} outside of any Lexgine namespace. This type export will be ignored.")

        source = source[:idx] + source[idx + len(CPP_INTERFACE_LOOKUP_KEYWORD) + 1:]

        offset = idx
        idx = source.find(CPP_INTERFACE_LOOKUP_KEYWORD, offset)

    return rv, source


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
    return rv


def fetch_parsed_class(class_ref: LexgineTypeReference, parsed_classes: Dict[str, CppClass]) -> CppClass:
    if class_ref.name not in parsed_classes or class_ref.line != parsed_classes[class_ref.name]["line_number"]:
        raise AssertionError(f'Unable to locate declaration for export reference "{class_ref.token_type} {class_ref.name}" in '
                             f'header "{export_billet.exporting_header_path}"')
    return parsed_classes[class_ref.name]


def is_ioc_class(cpp_class: CppClass, exported_methods: List[CppMethod]) -> bool:
    if len(exported_methods) > 0 or len(cpp_class["inherits"]) > 0 or len(cpp_class["properties"]["private"]) > 0 \
       or len(cpp_class["properties"]["protected"]) > 0:
        return True

    class_methods = cpp_class["methods"]
    for m in class_methods["public"] + class_methods["protected"] + class_methods["private"]:
        if m["final"] or m["override"] or m["virtual"] or m["pure_virtual"] or (m["constructor"] and m["name"] == cpp_class["name"]):
            return True

    return False


def map_inherited_type(type: str, replaced_types_dictionary: dict) -> Optional[str]:
    for k in replaced_types_dictionary:
        if is_subname(type, k):
            return replaced_types_dictionary[k]
    return None


def fetch_list_of_exported_methods_for_class(export_billet: PreprocessorHeaderBillet, parsed_class: CppClass) -> List[CppMethod]:
    rv = []
    parsed_public_methods_in_class = {e["line_number"]: idx for idx, e in enumerate(parsed_class["methods"]["public"])}
    discovered_functions = []
    for export_ref in export_billet.exported_types_set:
        if export_ref.token_type == TokenType.FUNCTION and export_ref.line in parsed_public_methods_in_class.keys():
            rv.append(parsed_class["methods"]["public"][parsed_public_methods_in_class[export_ref.line]])
            discovered_functions.append(export_ref)
    export_billet.exported_types_set.difference_update(discovered_functions)
    return rv


def fetch_list_of_exported_flags_for_class(source: str, export_billet: PreprocessorHeaderBillet, parsed_class: CppClass) -> List[LexgineFlags]:
    rv = []
    public_flag_declarations_found_in_class = [e["line_number"] for e in parsed_class["methods"]["public"] if e["name"] == "BEGIN_FLAGS_DECLARATION"
                                               and e["constructor"] is True]
    discovered_flag_declarations = []
    for export_ref in export_billet.exported_types_set:
        if export_ref.token_type == TokenType.FLAGS and export_ref.line in public_flag_declarations_found_in_class:
            rv.append(LexgineFlags(source, get_line_offset(source, export_ref.line - 1), export_ref.name))
            discovered_flag_declarations.append(export_ref)
    export_billet.exported_types_set.difference_update(discovered_flag_declarations)
    return rv


class FunctionKind(Enum):
    DEFAULT = 0
    CONSTRUCTOR = 1
    DESTRUCTOR = 2


def get_mangled_function_name(namespace: LexgineNamespace, parent_class: CppClass, function: Optional[CppMethod], kind: FunctionKind = FunctionKind.DEFAULT) -> str:
    namespace_parts = namespace.definition.split(sep='::')
    namespace_token_part = reduce(lambda x, y: x + y[0].upper() + y[1:], namespace_parts[1:], namespace_parts[0])
    class_token_part = parent_class["name"]

    method_token_part = ""
    parameters_token_part = ""
    if (kind == FunctionKind.DEFAULT or kind == FunctionKind.CONSTRUCTOR) and function is not None:
        def process_type(type_token: str) -> str:
            rv = type_token.replace('::', '__').replace(' ', '').replace('&', 'LREF').replace('&&', 'HREF').replace('const', 'CONST').replace('volatile', 'VOLATILE')
            return rv

        method_token_part = function["name"]
        parameters_token_part = reduce(lambda x, y: x + f"YY{process_type(y['type'])}", function["parameters"], "")

    if kind == FunctionKind.CONSTRUCTOR:
        method_token_part += "CreateInstance"
    elif kind == FunctionKind.DESTRUCTOR:
        method_token_part = "DestroyInstance"

    name_string = f"{namespace_token_part}XXXX{class_token_part}XXXX{method_token_part}{parameters_token_part}"
    return name_string


def generate_export_function_definition(namespace: LexgineNamespace, parent_class: CppClass, function: Optional[Union[CppMethod, List[CppMethod]]]) -> str:
    functions_to_define: List[CppMethod] = []
    generate_construction_destruction_code = False
    if isinstance(function, list):
        functions_to_define = function
    elif isinstance(function, CppMethod):
        functions_to_define = [function]
    elif function is None:
        functions_to_define = [e for e in parent_class["methods"]["public"] if e["constructor"] is True and e["name"] == parent_class["name"]]
        generate_construction_destruction_code = True

    rv = ""
    for foo in functions_to_define:
        return_type_string = "void*" if generate_construction_destruction_code is True else foo["rtnType"]
        parameters_string = "("
        if generate_construction_destruction_code is True:
            name_string = get_mangled_function_name(namespace, parent_class, foo, FunctionKind.CONSTRUCTOR)
        else:
            name_string = get_mangled_function_name(namespace, parent_class, foo, FunctionKind.DEFAULT)
            parameters_string += "void* p_instance"
            if len(foo["parameters"]):
                parameters_string += ', '

        param_call_list = ""
        for is_last, param in signal_last(foo["parameters"]):
            parameters_string += f"{param['type']} {param['name']}"
            param_call_list += param['name']
            if is_last is False:
                parameters_string += ", "
                param_call_list += ", "
        parameters_string += ")"

        rv += f"LEXGINE_API {return_type_string} {name_string}{parameters_string}\n"
        if generate_construction_destruction_code is True:
            rv += "{\n\t return new " + parent_class['name'] + "{" + param_call_list + "};\n}\n\n"
        else:
            rv += "{\n\treinterpret_cast<" + f"{parent_class['name']}*>(p_instance)->{foo['name']}({param_call_list});\n" + "}\n\n"

    if generate_construction_destruction_code is True:
        # Generate destruction code
        destructor_name_string = get_mangled_function_name(namespace, parent_class, None, FunctionKind.DESTRUCTOR)
        rv += f"LEXGINE_API void {destructor_name_string}(void* p_instance)\n" + "{\n\tdelete reinterpret_cast<" + f"{parent_class['name']}*>(p_instance);\n" + "}\n\n"

    return rv


def create_hpp_guard(namespace: LexgineNamespace, cpp_class: CppClass) -> str:
    namespace_parts = namespace.definition.split('::')
    hpp_guard = reduce(lambda x, y: f"{x}_{y.upper()}", namespace_parts[1:], namespace_parts[0].upper()) \
        + f"_{convert_camel_case_to_snake_case(cpp_class['name']).upper()}_H"
    return hpp_guard


def create_import_source_for_data_class(parent_namespace: LexgineNamespace, cpp_class: CppClass, class_type: TokenType,
                                        preprocessor_ready_header_source: str) -> LexgineRuntimeLinkInterface:
    assert class_type == TokenType.CLASS or class_type == TokenType.STRUCT

    header_template_string = """// This header is automatically created by Lexgine Runtime API generation system and is not intended to be modified by the end user.
//
//
// Copyright $year Alex Bibov
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// 
//    1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 
//    2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the 
//       documentation and/or other materials provided with the distribution.
// 
//    3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this 
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#ifndef $hpp_guard
#define $hpp_guard

namespace $parent_namespace{

$token_type $data_class_definition;

}

#endif $hpp_guard  
    """

    definition_begin_offset = get_line_offset(preprocessor_ready_header_source, cpp_class["line_number"] - 1)
    definition_begin_offset = preprocessor_ready_header_source.find(str(class_type.value), definition_begin_offset) + len(class_type.value)
    _, definition_end_offset = extract_scope(preprocessor_ready_header_source, definition_begin_offset, ScopeType.BRACES)

    definition_str = preprocessor_ready_header_source[definition_begin_offset:definition_end_offset+1].strip(' \t\n\r')
    header = Template(header_template_string).substitute(year=datetime.date.today().year, hpp_guard=create_hpp_guard(parent_namespace, cpp_class),
                                                         parent_namespace=parent_namespace.definition, token_type=class_type.value,
                                                         data_class_definition=definition_str)
    return LexgineRuntimeLinkInterface(None, header)


def create_export_source_for_ioc_class(parent_namespace: LexgineNamespace, cpp_class: CppClass,
                                       exporting_header_name: str, exported_methods: List[CppMethod]) -> LexgineDllExportInterface:
    source_template_string = """// This source has been automatically generated by Lexgine Runtime API generation system and is not intended to be modified by the end user.
// 
//
// Copyright $year Alex Bibov
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// 
//    1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 
//    2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the 
//       documentation and/or other materials provided with the distribution.
// 
//    3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this 
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <${exporting_header}>
#include <preprocessor_tokens.h>

namespace $exporting_namespace::_shared_symbols{

extern "C" {

$source_body

}

}
"""
    source = f"{generate_export_function_definition(parent_namespace, cpp_class, None)}\n\n{generate_export_function_definition(parent_namespace, cpp_class, exported_methods)}"
    exporting_header = f"{parent_namespace.definition.replace('::', '/').replace('lexgine', 'engine')}/{exporting_header_name}"
    cpp_exporting_source = string.Template(source_template_string).substitute(year=datetime.date.today().year,
                                                                              exporting_header=exporting_header,
                                                                              exporting_namespace=parent_namespace.definition, source_body=source)
    return LexgineDllExportInterface(cpp_exporting_source)


def create_import_source_for_ioc_class(parent_namespace: LexgineNamespace, cpp_class: CppClass, class_type: TokenType, includes_list: List[str],
                                       exported_flags: List[LexgineFlags], exported_methods: List[CppMethod]) -> LexgineRuntimeLinkInterface:
    hpp_template_string = """// This header has been automatically generated by Lexgine Runtime API generation system and is not intended to be modified by the end user.
// 
//
// Copyright $year Alex Bibov
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// 
//    1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 
//    2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the 
//       documentation and/or other materials provided with the distribution.
// 
//    3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this 
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef $hpp_guard
#define $hpp_guard
#endif

#include <windows.h>
$includes_list

namespace $namespace{

$class_token $class_name $class_qualifier: public LexgineObject
{
public:    // public properties
$public_properties_setters_getters_declarations
    
public:    // flags
$exported_flags
    
public:    // runtime linking infrastructure
    static LinkResult link(HMODULE module);    //! Runtime link interface
    ${class_name}* getNative() const;    //! Returns opaque pointer to the native runtime class
    
public:    // API methods
$api_methods
};

}
"""

    includes_list_src: str = reduce(lambda x, y: x + f"#include <{y}>\n", includes_list, "")

    supported_public_properties_list = []
    for p in cpp_class["properties"]["public"]:
        if p["static"] is True:
            print(f"WARNING: property {p['name']} is static. Static properties are not supported in IOC export types.")
            continue
        supported_public_properties_list.append(p)

    property_accessor_dummy_class_definition = f"class {cpp_class['name']}Dummy final\n" + "{\n" \
                                               + reduce(lambda x, y: x + f"\t{y['type']} {'const' if y['const'] is True else ''} {y['name']};\n",
                                                        supported_public_properties_list, "") + "};\n"

    public_properties_setters_getters_declarations = ""
    public_properties_setters_getters_definitions = ""
    for p in supported_public_properties_list:
        name = convert_snake_case_to_camel_case(p["name"])
        name = name[0].upper() + name[1:]
        public_properties_setters_getters_declarations += f"\t{p['type']} get{name}() const;\n"
        public_properties_setters_getters_definitions += f"{p['type']} {cpp_class['name']}::get{name}() const\n" + '{\n\t' \
            + f"return *reinterpret_cast<{p['type']}*>(reinterpret_cast<char*>(m_ptr) + offsetof({cpp_class['name']}Dummy, {p['name']}));\n" + '}\n'

        if p["const"] is False:
            public_properties_setters_getters_declarations += f"\tvoid set{name}(public_property_type_accessor<{p['type']}>::value_type value);\n"
            public_properties_setters_getters_definitions += f"void {cpp_class['name']}::set{name}(public_property_type_accessor<{p['type']}>::value_type value)\n" + '{\n\t' \
                + f"*reinterpret_cast<{p['type']}*>(reinterpret_cast<char*>(m_ptr) + offsetof({cpp_class['name']}Dummy, {p['name']})) = value;\n" + '}\n'

    api_methods_declarations = ""
    api_methods_definitions = ""
    api_linked_pointers = ""
    link_methods_call_list = "\t"
    constructors = [m for m in cpp_class["methods"]["public"] if m["constructor"] is True and m["name"] == cpp_class["name"]]
    for m in exported_methods + constructors:
        api_methods_declarations += f"\t{m['rtnType']} {m['name']}("
        api_methods_definitions += f"{m['rtnType']} {cpp_class['name']}::{m['name']}("
        call_list = ""
        mangled_method_name = get_mangled_function_name(parent_namespace, cpp_class, m, FunctionKind.DEFAULT if m["constructor"] is False else FunctionKind.CONSTRUCTOR)
        link_methods_call_list += f'api__{mangled_method_name} = reinterpret_cast<decltype(api__{mangled_method_name})>(linker.attemptLink("{mangled_method_name}"));\n\t'
        api_pointer_declaration = f"static {m['rtnType']}(LEXGINE_CALL* api__{mangled_method_name})("
        for s, p in signal_last(m["parameters"]):
            api_methods_declarations += f"{p['type']} {p['name']}{', ' if s is False else ''}"
            api_methods_definitions += f"{p['type']} {p['name']}{', ' if s is False else ''}"
            api_pointer_declaration += f"{p['type']}{', ' if s is False else ''}"
            call_list += f"unfold({p['name']}){', ' if s is False else ''}"
        api_methods_declarations += ')'
        api_methods_definitions += ')'
        api_pointer_declaration += ')'
        if m["const"] is True:
            api_methods_declarations += " const"
            api_methods_definitions += " const"
            api_pointer_declaration += " const"
        api_linked_pointers += f"{api_pointer_declaration} = nullptr;\n"
        if 'doxygen' in m:
            api_methods_declarations += f";    {m['doxygen']}\n"
        else:
            api_methods_declarations += ";\n"

        if m["constructor"] is True:
            api_methods_definitions += "\n\t: LexgineObject{" + f"api__{mangled_method_name}({call_list})" + "}\n{\t\n}\n\n"
        else:
            api_methods_definitions += "\n{\n\t" + f"{'return ' if m['rtnType'] != 'void' else ''}api__{mangled_method_name}({call_list});\n" + '}\n\n'

    # Link destructors
    mangled_method_name = get_mangled_function_name(parent_namespace, cpp_class, None, FunctionKind.DESTRUCTOR)
    api_linked_pointers += f"static void(LEXGINE_CALL* api__{mangled_method_name})(void*) = nullptr;\n"
    link_methods_call_list += f'api__{mangled_method_name} = reinterpret_cast<decltype(api__{mangled_method_name})>(linker.attemptLink("{mangled_method_name}"));\n'
    api_methods_declarations += f"\tvirtual void ~{cpp_class['name']}();"
    api_methods_definitions += f"void {cpp_class['name']}::~{cpp_class['name']}()\n" + "{\n\t" + f"api__{mangled_method_name}(getNative());\n" + "}"

    import_api_hpp = string.Template(hpp_template_string).substitute(
        year=datetime.date.today().year, hpp_guard=create_hpp_guard(parent_namespace, cpp_class),
        namespace=parent_namespace.definition,
        includes_list=includes_list_src,
        class_token=class_type.value, class_name=cpp_class["name"],
        class_qualifier=f"{'final' if cpp_class['final'] is True else ''}",
        public_properties_setters_getters_declarations=public_properties_setters_getters_declarations,
        exported_flags=reduce(lambda x, y: f"{x}\t{y.declaration_string}\n", exported_flags, ""),
        api_methods=api_methods_declarations
    )

    cpp_template_string = """// This source code has been automatically generated by Lexgine Runtime API generation system and is not intended to be modified by the end user.
// 
//
// Copyright $year Alex Bibov
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// 
//    1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 
//    2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the 
//       documentation and/or other materials provided with the distribution.
// 
//    3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this 
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <${api_name}.h>

namespace $namespace{

namespace{

$public_properties_memory_layout_declaration

$api_function_pointers_declarations

}

LinkResult ${class_name}::link(HMODULE module)
{
    LinkResult linker{module};

$link_methods_call_list
    
    return linker;
}

${class_name}* ${class_name}::getNative() const
{
    return reinterpret_cast<${class_name}*>(LexgineObject::getNative());
}

$public_properties_setters_getters_definitions

$api_methods_definitions

}
"""
    import_api_cpp = string.Template(cpp_template_string).substitute(year=datetime.date.today().year,
                                                                     api_name=f"{convert_camel_case_to_snake_case(cpp_class['name'])}",
                                                                     namespace=parent_namespace.definition,
                                                                     public_properties_memory_layout_declaration=property_accessor_dummy_class_definition,
                                                                     api_function_pointers_declarations=api_linked_pointers,
                                                                     class_name=cpp_class["name"],
                                                                     link_methods_call_list=link_methods_call_list,
                                                                     public_properties_setters_getters_definitions=public_properties_setters_getters_definitions,
                                                                     api_methods_definitions=api_methods_definitions)
    return LexgineRuntimeLinkInterface(source=import_api_cpp, header=import_api_hpp)


def create_link_api(ioc_interfaces: List[Tuple[LexgineNamespace, CppClass, Path]], write_directory: Path):
    link_api_source = """// This source code has been automatically generated by Lexgine Runtime API generation system and is not intended to be modified by the end user.
// 
//
// Copyright $year Alex Bibov
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// 
//    1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 
//    2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the 
//       documentation and/or other materials provided with the distribution.
// 
//    3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this 
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <link_lexgine_api.h>
$includes_list

namespace lexgine::api{

std::unordered_map<std::string, LinkResult> linkLexgineApi(std::wstring const& path_to_engine)
{
    std::unordered_map<std::string, LinkResult> rv{};
    HMODULE module = LoadLibraryW(path_to_engine.c_str());
    if(!module)
    {
        return rv;
    }
    
$link_calls_list
    return rv;
}

}

"""

    includes_list = reduce(lambda x, y: x + f"#include <{y[1].as_posix()}/{convert_camel_case_to_snake_case(y[0]['name'])}.h>\n", [(e[1], e[2]) for e in ioc_interfaces], "")
    link_calls_list = ""
    for namespace, cls, _ in ioc_interfaces:
        link_calls_list += "\t{\n\t\t" \
                           + f'auto link_result = {namespace.definition}::{cls["name"]}::link(module);\n\t\trv["{namespace.definition}::{cls["name"]}"] = link_result;\n' \
                           + "\t}\n"
    source = string.Template(link_api_source).substitute(year=datetime.date.today().year, includes_list=includes_list, link_calls_list=link_calls_list)

    with open((write_directory / "link_lexgine_api.cpp").as_posix(), 'w') as f:
        f.write(source)


print(arguments.headers)
headers = arguments.headers.split(sep=";")

ignored_hpp_paths = [Path(hpp) for hpp in configuration.get("ignored_headers", [])]
forced_includes = [Path(e) for e in configuration.get("forced_includes", [])]
inherited_types_mapping = configuration.get("inherited_types_api_mapping", {})

billets: List[PreprocessorHeaderBillet] = []

engine_path = None
for hpp in headers:
    source_hpp_path = Path(hpp)

    engine_path_candidate = extract_engine_path(source_hpp_path)
    if engine_path is None:
        engine_path = engine_path_candidate

    if engine_path_candidate is None or engine_path_candidate != engine_path:
        raise RuntimeError("Unable to determine engine path")

    if is_hpp_path_ignored(source_hpp_path, ignored_hpp_paths) is True:
        continue

    with open(source_hpp_path.as_posix(), 'r') as f:
        header_source_code = f.read()

    lexgine_namespaces = get_lexgine_namespaces(header_source_code)
    exported_types_set, header_source_code = list_raw_lexgine_export_references(header_source_code, lexgine_namespaces)
    billets.append(PreprocessorHeaderBillet(exporting_header_path=source_hpp_path, exported_types_set=exported_types_set, preprocessor_ready_header_source=header_source_code))

ioc_interfaces: List[Tuple[LexgineNamespace, CppClass, Path]] = []
for export_billet in billets:
    header_source = export_billet.preprocessor_ready_header_source
    header_name = str(export_billet.exporting_header_path.stem)

    try:
        parsed_header = CppHeader(header_source, argType="string")
    except CppParseError as e:
        print(f'Problem parsing header "{export_billet.exporting_header_path.as_posix()}": {e}')
        continue

    includes = [inc.strip('"<>') for inc in parsed_header.includes]
    classes = parsed_header.classes
    functions = parsed_header.functions

    target_api_header_path = create_api_path(export_billet.exporting_header_path)

    if len(export_billet.exporting_header_path.parts) > 1 and len(export_billet.exported_types_set) > 0:
        (engine_path / target_api_header_path).mkdir(parents=True, exist_ok=True)

    forced_includes_as_strings = [e.as_posix() for e in forced_includes]
    filtered_unique_includes = set(forced_includes_as_strings + includes)

    exported_classes = [e for e in export_billet.exported_types_set if e.token_type == TokenType.STRUCT or e.token_type == TokenType.CLASS]
    exported_namespaces: List[LexgineNamespace] = get_lexgine_namespaces(export_billet.preprocessor_ready_header_source)
    for e in exported_classes:
        parsed_class: CppClass = fetch_parsed_class(e, classes)
        parent_namespace: LexgineNamespace = get_lexgine_namespace_for_location(exported_namespaces, parsed_class["line_number"] - 1)
        exported_methods_in_class: List[CppMethod] = fetch_list_of_exported_methods_for_class(export_billet, parsed_class)
        if is_ioc_class(parsed_class, exported_methods_in_class):
            exported_flags_in_class = fetch_list_of_exported_flags_for_class(header_source, export_billet, parsed_class)
            export_interface = create_export_source_for_ioc_class(parent_namespace, parsed_class, export_billet.exporting_header_path.name, exported_methods_in_class)
            import_interface = create_import_source_for_ioc_class(parent_namespace, parsed_class, e.token_type, list(filtered_unique_includes),
                                                                  exported_flags_in_class, exported_methods_in_class)

            export_interface_path = (export_billet.exporting_header_path.parent / "_shared_symbols")
            export_interface_path.mkdir(parents=True, exist_ok=True)
            export_interface.write(export_interface_path, convert_camel_case_to_snake_case(parsed_class["name"]))

            ioc_interfaces.append((parent_namespace, parsed_class, target_api_header_path))
        else:
            import_interface = create_import_source_for_data_class(parent_namespace, parsed_class, e.token_type, export_billet.preprocessor_ready_header_source)
        import_interface.write(engine_path / target_api_header_path, convert_camel_case_to_snake_case(parsed_class["name"]))
create_link_api(ioc_interfaces, engine_path / "api")






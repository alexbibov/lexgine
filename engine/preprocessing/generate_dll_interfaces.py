import argparse
import datetime
from collections import namedtuple
from pathlib import Path
from CppHeaderParser import CppHeader, CppParseError
from typing import List, Optional

INTERFACE_LOOKUP_KEYWORD = "LEXGINE_CPP_API"

argument_parser = argparse.ArgumentParser(description="Lexgine DLL interface generator")
argument_parser.add_argument("--headers", required=True, help="List of paths to the C++ headers to be parsed",
                             dest="headers")
argument_parser.add_argument("--output", required=True, help="Output directory where to write the generated interfaces",
                             dest="output")
arguments = argument_parser.parse_args()

LexgineNamespace = namedtuple("LexgineNamespace", ["definition", "begin_line_number", "end_line_number", "begin_index", "end_index"])
LexgineDLLInterface = namedtuple("LexgineDLLInterface", ["cpp", "hpp"])


def locate_closing_brace(source: str, opening_brace_offset: int) -> int:
    counter = 1
    for i in range(opening_brace_offset + 1, len(source)):
        if source[i] == '{':
            counter += 1
        elif source[i] == '}':
            counter -= 1
        if counter == 0:
            break
    return i


def count_lines_till_index(source: str, offset: int, index: int) -> int:
    return len(source[offset:index+1].splitlines())


def search_for_lexgine_namespace(header_source_code: str, offset: int) -> Optional[LexgineNamespace]:
    search_offset = offset
    lexgine_namespace_start_idx = -1
    while lexgine_namespace_start_idx == -1 and search_offset < len(header_source_code):
        idx = header_source_code.find("namespace", search_offset)
        if idx == -1:
            break

        lexgine_namespace_start_idx = idx + len("namespace")
        subsource = header_source_code[lexgine_namespace_start_idx:].lstrip()
        if not subsource.startswith("lexgine"):
            search_offset = lexgine_namespace_start_idx
            lexgine_namespace_start_idx = -1

    if lexgine_namespace_start_idx == -1:
        return None
    else:
        lexgine_namespace_begin_idx = header_source_code.find('{', lexgine_namespace_start_idx)
        lexgine_namespace_end_idx = locate_closing_brace(header_source_code, lexgine_namespace_begin_idx)
        lexgine_namespace_definition_token = header_source_code[lexgine_namespace_start_idx:lexgine_namespace_begin_idx].strip()

        lexgine_namespace_begin_line = count_lines_till_index(header_source_code, 0, lexgine_namespace_begin_idx)
        lexgine_namespace_end_line = count_lines_till_index(header_source_code, 0, lexgine_namespace_end_idx)

        return LexgineNamespace(lexgine_namespace_definition_token,
                                lexgine_namespace_begin_line, lexgine_namespace_end_line,
                                lexgine_namespace_begin_idx, lexgine_namespace_end_idx)


def get_lexgine_namespaces(header_source_code: str) -> List[LexgineNamespace]:
    rv = []
    new_lexgine_namespace = search_for_lexgine_namespace(header_source_code, 0)
    while new_lexgine_namespace is not None:
        rv.append(new_lexgine_namespace)
        new_lexgine_namespace = search_for_lexgine_namespace(header_source_code, new_lexgine_namespace.end_index + 1)
    return rv


def get_lexgine_namespace_for_location(lexgine_namespaces: List[LexgineNamespace], location_index: int) -> Optional[LexgineNamespace]:
    for namespace in lexgine_namespaces:
        if namespace.begin_line_number <= location_index <= namespace.end_line_number:
            return namespace
    return None


headers = arguments.headers.split(sep=";")
for hpp in headers:
    hpp_path = Path(hpp)
    interfaces = {}
    try:
        parsed_header = CppHeader(hpp_path.as_posix())
        classes = parsed_header.classes
    except CppParseError as e:
        # print(e)
        continue

    with open(hpp_path.as_posix(), 'r') as f:
        header_source_code = f.read()

    lexgine_namespaces = get_lexgine_namespaces(header_source_code)

    preamble = \
        f"""// THIS FILE WAS GENERATED BASED ON '{hpp_path.as_posix()}' BY AUTOMATIC PARSER.
// THIS IS A PART OF LEXGINE DLL C++ INTERFACE. DO NOT MODIFY OR DELETE.
// COPYRIGHT(C) ALEXANDER BIBOV, {datetime.datetime.now().year}\n\n\n"""
    source_hpp = preamble
    source_cpp = preamble

    define_guard = f"LEXGINE_RUNTIME_{hpp_path.stem.upper()}_H"

    source_hpp += f"#ifndef {define_guard}\n" \
                  + f"#define {define_guard}\n\n"

    source_cpp += f'#include "{hpp_path.stem}.h"\n' \
                  + f'#include "{hpp_path.as_posix()}"\n' \
                  + "using namespace lexgine;\n"

    for include_path in parsed_header.includes:
        source_hpp += f"#include {include_path}\n"
    source_hpp += '\n'

    found_lexgine_source_code = False

    for name, body in classes.items():
        if name.startswith(INTERFACE_LOOKUP_KEYWORD):
            namespace = get_lexgine_namespace_for_location(lexgine_namespaces, body["line_number"])
            if namespace is not None and hpp_path.stem not in interfaces:

                found_lexgine_source_code = True

                class_name_wo_prefix = name[len(INTERFACE_LOOKUP_KEYWORD):]
                target_runtime_namespace = namespace.definition.replace('lexgine', 'lexgine::runtime')
                source_cpp += f"using namespace {target_runtime_namespace};\n\n"
                source_hpp += f"namespace {target_runtime_namespace} {{\n\n" \
                              + f"class LEXGINE_API {class_name_wo_prefix}\n{{\n\t" \
                              + "friend class EngineManager;\n\n" \
                              + "public:\n\n\t"

                print(f'Found class "{class_name_wo_prefix}" in namespace "{namespace.definition}"')

                for e in body["methods"]["public"]:
                    if e["rtnType"].startswith(INTERFACE_LOOKUP_KEYWORD):
                        print(f"Discovering method \"{e['debug'][len(INTERFACE_LOOKUP_KEYWORD) + 1:]}\"")

                        source_hpp += e["doxygen"] + "\n\t"
                        return_type = e["rtnType"][len(INTERFACE_LOOKUP_KEYWORD) + 1:]
                        source_hpp += return_type + ' ' + e["name"] + '('
                        source_cpp += return_type + ' ' + class_name_wo_prefix + '::' + e["name"] + '('
                        for idx in range(0, len(e["parameters"])):
                            p = e["parameters"][idx]
                            parameter_type_and_name = p["type"] + ' ' + p["name"]
                            source_hpp += parameter_type_and_name
                            source_cpp += parameter_type_and_name
                            if idx < len(e["parameters"]) - 1:
                                source_hpp += ", "
                                source_cpp += ", "
                        const_token = (" const" if e["const"] else "")
                        source_hpp += ")" + const_token + ";\n\n\t"
                        source_cpp += ")" + const_token + '\n'\
                            + f"{{\n\t{'return ' if return_type != 'void' else ''}reinterpret_cast<{namespace.definition}::{class_name_wo_prefix}{const_token}*>(m_ptr)->{e['name']}("
                        for idx in range(0, len(e["parameters"])):
                            p = e["parameters"][idx]
                            source_cpp += p["name"]
                            if idx < len(e["parameters"]) - 1:
                                source_cpp += ", "
                        source_cpp += ");\n}\n\n"

                source_hpp += "\nprivate:\n\t" \
                    + f"{class_name_wo_prefix}(void*);\n\n"
                source_cpp += f"{class_name_wo_prefix}::{class_name_wo_prefix}(void* internal_ptr)\n" \
                    + ": m_ptr{internal_ptr}\n{\n}\n"

                source_hpp += "private:\n\tvoid* m_ptr;\n};\n\n"

    if found_lexgine_source_code is True:
        source_hpp += "}\n\n#endif\n"
        interfaces[hpp_path.stem] = LexgineDLLInterface(source_cpp, source_hpp)
        found_lexgine_source_code = False

    for name, descriptor in interfaces.items():
        with open((Path(arguments.output)/(name + ".h")).as_posix(), 'w') as f:
            f.write(descriptor.hpp)
        with open((Path(arguments.output)/(name + ".cpp")).as_posix(), 'w') as f:
            f.write(descriptor.cpp)








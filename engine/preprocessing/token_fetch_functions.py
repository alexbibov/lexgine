from typing import Dict, Optional, List, Union
from CppHeaderParser import CppClass, CppEnum, CppUnion, CppMethod

import common_types
import util


def fetch_parsed_class(
        class_ref: common_types.LexgineTypeReference,
        parsed_classes: Dict[str, CppClass]
) -> Optional[CppClass]:
    if class_ref.name not in parsed_classes:
        return None
    parsed_class = parsed_classes[class_ref.name]
    parsed_line = parsed_class.get("line_number", -1)
    if parsed_line > 0 and class_ref.line > 0 and class_ref.line != parsed_line:
        return None
    return parsed_class


def fetch_parsed_enum_or_union(
        export_billet: common_types.PreprocessorHeaderBillet,
        type_ref: common_types.LexgineTypeReference,
        parsed_types: List[CppEnum],
        parsed_unions: Dict[str, CppUnion]
) -> Union[CppEnum, CppUnion]:
    for e in parsed_types:
        if e["name"] == type_ref.name and (e.get("line_number", -1) <= 0 or e["line_number"] == type_ref.line):
            return e
    for k, v in parsed_unions.items():
        if (k == type_ref.name or v["name"]
                == type_ref.name) and (v.get("line_number", -1) <= 0 or v["line_number"] == type_ref.line):
            return v
    raise AssertionError(
        f'Unable to locate declaration for export reference "{type_ref.token_type} {type_ref.name}" in '
        f'header "{export_billet.exporting_header_path}"')


def fetch_list_of_exported_methods_for_class(export_billet: common_types.PreprocessorHeaderBillet,
                                             parsed_class: CppClass) -> List[CppMethod]:
    rv = []
    parsed_public_methods = parsed_class["methods"]["public"]
    parsed_public_methods_in_class = {
        e["line_number"]: idx
        for idx, e in enumerate(parsed_public_methods) if e.get("line_number", -1) > 0
    }
    discovered_functions = []
    for export_ref in export_billet.exported_types_set:
        if (export_ref.token_type == common_types.TokenType.FUNCTION
                and (export_ref.line in parsed_public_methods_in_class.keys()
                     or any(m["name"] == export_ref.name for m in parsed_public_methods))):
            if export_ref.line in parsed_public_methods_in_class.keys():
                rv.append(parsed_public_methods[parsed_public_methods_in_class[export_ref.line]])
            else:
                for m in parsed_public_methods:
                    if m["name"] == export_ref.name and m not in rv:
                        rv.append(m)
            discovered_functions.append(export_ref)
    export_billet.exported_types_set.difference_update(discovered_functions)
    return rv


def fetch_list_of_exported_flags_for_class(source: str, export_billet: common_types.PreprocessorHeaderBillet,
                                           parsed_class: CppClass) -> List[common_types.LexgineFlags]:
    rv = []
    public_flag_declarations_found_in_class = [
        e["line_number"] for e in parsed_class["methods"]["public"]
        if e["name"] == "BEGIN_FLAGS_DECLARATION" and e["constructor"] is True
    ]
    discovered_flag_declarations = []
    for export_ref in export_billet.exported_types_set:
        if export_ref.token_type != common_types.TokenType.FLAGS:
            continue
        if (export_ref.line in public_flag_declarations_found_in_class
                or len(public_flag_declarations_found_in_class) == 0):
            rv.append(common_types.LexgineFlags(source, util.get_line_offset(source, export_ref.line - 1), export_ref.name))
            discovered_flag_declarations.append(export_ref)
    export_billet.exported_types_set.difference_update(discovered_flag_declarations)
    return rv


def fetch_list_of_exported_enums_in_class(export_billet: common_types.PreprocessorHeaderBillet,
                                          parsed_class: CppClass) -> List[CppEnum]:
    rv: List[CppEnum] = []
    public_enums = parsed_class["enums"]["public"]
    public_enums_decl_line_idx_map = {e["line_number"]: idx for idx, e in enumerate(public_enums)
                                      if e.get("line_number", -1) > 0}
    discovered_enum_declarations = []
    for export_ref in export_billet.exported_types_set:
        if export_ref.token_type == common_types.TokenType.ENUM:
            if (export_ref.line in public_enums_decl_line_idx_map
                    and export_ref.name == public_enums[public_enums_decl_line_idx_map[export_ref.line]]["name"]):
                rv.append(public_enums[public_enums_decl_line_idx_map[export_ref.line]])
            else:
                for e in public_enums:
                    if e["name"] == export_ref.name and e not in rv:
                        rv.append(e)
            discovered_enum_declarations.append(export_ref)
    export_billet.exported_types_set.difference_update(discovered_enum_declarations)
    return rv


def fetch_list_of_exported_unions_in_class(export_billet: common_types.PreprocessorHeaderBillet,
                                           parsed_class: CppClass) -> List[CppUnion]:
    rv: List[CppUnion] = []
    public_unions = [e for e in parsed_class["nested_classes"] if e["access_in_parent"] == "public"]
    public_unions_decl_line_idx_map = {e["line_number"]: idx for idx, e in enumerate(public_unions)
                                       if e.get("line_number", -1) > 0}
    discovered_union_declarations = []
    for export_ref in export_billet.exported_types_set:
        if export_ref.token_type == common_types.TokenType.UNION:
            if (export_ref.line in public_unions_decl_line_idx_map
                    and export_ref.name == public_unions[public_unions_decl_line_idx_map[export_ref.line]]["name"]):
                rv.append(public_unions[public_unions_decl_line_idx_map[export_ref.line]])
            else:
                for e in public_unions:
                    if e["name"] == export_ref.name and e not in rv:
                        rv.append(e)
            discovered_union_declarations.append(export_ref)
    export_billet.exported_types_set.difference_update(discovered_union_declarations)
    return rv


import re
from typing import List, Tuple, Optional
from CppHeaderParser import CppClass, CppMethod
from functools import reduce

import common_types
import util


def get_namespace_and_type_name_from_type_desc(type_desc: str) -> Tuple[str, str]:
    idx = type_desc.rfind("::")
    type_namespace = type_desc[:idx] if idx >= 0 else ""
    type_class_name_and_modifiers = type_desc[idx +
                                              2:] if idx >= 0 else type_desc

    idx = type_desc.find(" ")
    type_class_name = type_class_name_and_modifiers[:idx] if idx >= 0 else type_class_name_and_modifiers
    return type_namespace, type_class_name


def get_context_nested_namespace_variations_for_type_desc(context_namespace: common_types.LexgineNamespace,
                                                          type_desc: str) -> Tuple[List[str], str]:
    type_namespace, type_name = get_namespace_and_type_name_from_type_desc(type_desc)
    context_namespace_nested_tokens = context_namespace.definition.split("::")
    lookup_prefixes = reduce(
        lambda x, y: x + [f"{x[-1]}::{y}"], context_namespace_nested_tokens[1:], [context_namespace_nested_tokens[0]]
    )
    if len(type_namespace) > 0:
        lookup_namespaces = [type_namespace] + [f"{e}::{type_namespace}" for e in lookup_prefixes]
    else:
        lookup_namespaces = lookup_prefixes
    return lookup_namespaces, type_name


def get_full_qualified_name(namespace_desc: str, type_desc: str) -> str:
    return f"{namespace_desc}::{type_desc}" if len(namespace_desc) > 0 else type_desc


def is_ioc_class(cpp_class: CppClass, exported_methods: List[CppMethod]) -> bool:
    if len(exported_methods) > 0 or len(cpp_class["inherits"]) > 0 or len(cpp_class["properties"]["private"]) > 0 \
       or len(cpp_class["properties"]["protected"]) > 0:
        return True

    class_methods = cpp_class["methods"]
    for m in class_methods["public"] + class_methods["protected"] + class_methods["private"]:
        if (m["final"] or m["override"] or m["virtual"] or m["pure_virtual"]
                or m["constructor"] and m["name"] == cpp_class["name"]):
            return True

    return False


def is_ioc_type_desc(
        list_of_ioc_interfaces: List[common_types.ExportedResourceDescriptor],
        context_namespace: common_types.LexgineNamespace,
        type_desc: str
) -> Tuple[bool, Optional[common_types.ExportedResourceDescriptor]]:
    lookup_namespaces, type_name = get_context_nested_namespace_variations_for_type_desc(context_namespace, type_desc)
    qualifying_interfaces = []
    for e in list_of_ioc_interfaces:
        if e.parsed_class["name"] == type_name and e.parent_namespace.definition in lookup_namespaces:
            qualifying_interfaces.append(e)

    if len(qualifying_interfaces) > 1:
        print(
            f"WARNING: type look-up for type {type_name} resolves to more than single namespace. "
            f"Is there a naming collision?"
        )
        return True, qualifying_interfaces[0]
    elif len(qualifying_interfaces) == 0:
        return False, None
    else:  # len(qualifying_interface) == 1
        return True, qualifying_interfaces[0]


def is_pointer_type(type_desc: str) -> bool:
    return "*" in type_desc


def is_reference_type(type_desc: str) -> bool:
    return "&&" in type_desc or "&" in type_desc


def is_const(type_desc: str) -> bool:
    return "const" in type_desc


def is_volatile(type_desc: str) -> bool:
    return "volatile" in type_desc


def is_pointer_to_function_type(type_desc: str) -> bool:
    if '(' not in type_desc:
        return False

    all_tokens = []
    d = 0
    idx = 0
    for next_idx, c in enumerate(type_desc):
        if c == '(':
            d += 1
            if d == 1:
                all_tokens.append(type_desc[idx:next_idx])
                idx = next_idx + 1
        if c == ')':
            if d == 1:
                all_tokens.append(type_desc[idx:next_idx])
                idx = next_idx + 1
            d -= 1
    all_tokens = [t.strip(" \t\r\n") for t in all_tokens]
    all_tokens = [e for e in all_tokens if len(e) > 0]
    if len(all_tokens) != 3 and len(all_tokens) != 4:
        return False
    if len(all_tokens) == 4 and all_tokens[-1] != "const" and all_tokens[
            -1] != "volatile":
        return False

    type_ref_prefix_modifier_re = re.compile(
        r"((const|volatile)\s*)?[\w\d_:<>]+\s*(&+|\*+)?")
    type_ref_suffix_modifier_re = re.compile(
        r"[\w\d_:<>]+(\s*(const|volatile))?\s*(&+|\*+)?")
    if type_ref_prefix_modifier_re.fullmatch(
            all_tokens[0]) is None and type_ref_suffix_modifier_re.fullmatch(
                all_tokens[0]) is None:
        return False

    function_pointer_calling_convention_re = re.compile(
        r"[\w\d_:<>]*\s*\*\s*([\w\d_]+)?")
    if function_pointer_calling_convention_re.fullmatch(all_tokens[1]) is None:
        return False

    function_arguments_list = all_tokens[2].split(sep=',')
    function_arguments_list = [
        e.strip(" \t\r\n") for e in function_arguments_list
    ]
    function_argument_name_re = re.compile(r"[\w\d_]+")
    for a in function_arguments_list:
        t = a
        if type_ref_prefix_modifier_re.fullmatch(
                t) is None and type_ref_suffix_modifier_re.fullmatch(
                    t) is None:
            # t is not a type, it can be a type + name token
            for idx, s in enumerate(reversed(a)):
                if s == ' ':
                    break
            if s != ' ':
                return False
            else:
                t = a[:len(a) - idx - 1].strip(" \t\r\n")
                n = a[len(a) - idx:].strip(" \t\r\n")
                if function_argument_name_re.fullmatch(n) is None:
                    return False
    return True


def is_primitive_type(type_desc: str) -> bool:
    character_types = [
        "char", "wchar_t", "char8_t", "char16_t", "char32_t", "signed char",
        "unsigned char"
    ]
    integral_types = [
        "short", "short int", "signed short", "signed short int",
        "unsigned short", "unsigned short int", "int", "signed", "signed int",
        "unsigned", "unsigned int", "long", "long int", "signed long",
        "signed long int", "unsigned long", "unsigned long int", "long long",
        "long long int", "signed long long", "signed long long int",
        "unsigned long long", "unsigned long long int"
    ]
    floating_point_types = ["float", "double", "long double"]
    sized_types = [
        "int8_t", "int16_t", "int32_t", "int64_t", "int_fast8_t",
        "int_fast16_t", "int_fast32_t", "int_fast64_t", "int_least8_t",
        "int_least16_t", "int_least32_t", "int_least64_t", "intmax_t",
        "intptr_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "uint_fast8_t", "uint_fast16_t", "uint_fast32_t", "uint_fast64_t",
        "uint_least8_t", "uint_least16_t", "uint_least32_t", "uint_least64_t",
        "uintmax_t", "uintptr_t", "size_t"
    ]
    sized_types += [f"std::{e}" for e in sized_types]
    fundamental_types = [
        "void", "bool"
    ] + character_types + integral_types + floating_point_types + sized_types
    return type_desc in fundamental_types


def is_raw_pointer_to_ioc(
        type_desc: str,
        list_of_ioc_interfaces: List[common_types.ExportedResourceDescriptor],
        namespace: common_types.LexgineNamespace
) -> Tuple[bool, Optional[common_types.ExportedResourceDescriptor]]:
    idx = type_desc.rfind('*')
    if idx == -1:
        return False, None
    type_desc = type_desc[:idx].replace(' ', '')

    if is_primitive_type(type_desc) or is_pointer_type(
            type_desc) or is_reference_type(type_desc):
        return False, None

    return is_ioc_type_desc(list_of_ioc_interfaces, namespace, type_desc)


def is_unique_pointer_to_ioc(
        type_desc: str,
        list_of_ioc_interfaces: List[common_types.ExportedResourceDescriptor],
        namespace: common_types.LexgineNamespace
) -> Tuple[bool, Optional[common_types.ExportedResourceDescriptor]]:
    type_desc = type_desc.replace(' ', '')
    shared_ptr_re = re.compile(
        r"(std::)?unique_ptr<(?P<shared_ptr_type_desc>.+)>")

    m = shared_ptr_re.match(type_desc)
    if m is None:
        return False, None
    type_desc = m["shared_ptr_type_desc"].replace(' ', '')
    return is_ioc_type_desc(list_of_ioc_interfaces, namespace, type_desc)

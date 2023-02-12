import datetime
from typing import List, Optional, Union, Dict, Tuple
from CppHeaderParser import CppClass, CppMethod, CppEnum, CppUnion, CppVariable
from functools import reduce
from pathlib import Path
from string import Template
from enum import Enum

import global_names
import common_types
import parser_helpers
import util
from template_helpers import get_template_descriptor, create_hpp_guard


def replace_fp_type_with_using_directive(
        type_desc: str,
        parent_class: CppClass,
        function: Optional[CppMethod],
        using_directives: dict,
        using_dependency_list: set
) -> str:
    if parser_helpers.is_pointer_to_function_type(type_desc):
        if type_desc in using_directives:
            using_dependency_list.add((using_directives[type_desc], type_desc))
            return using_directives[type_desc]
        else:
            raise AssertionError(
                f"Parameter type '{type_desc}' in declaration of function {parent_class['name']}::"
                f"{function['name'] if function is not None else parent_class['name']} is not aliased by a using "
                f"directive within the enclosing class. Function pointer types used within the list of arguments in an "
                f"exported function must be always aliased."
            )
    else:
        return type_desc


def get_compatible_type_desc_for_output_parameter(type_desc: str, is_ioc_type: bool) -> str:
    assert (parser_helpers.is_primitive_type(type_desc) is False
            and parser_helpers.is_pointer_type(type_desc) is False
            and parser_helpers.is_reference_type(type_desc) is False)
    const = parser_helpers.is_const(type_desc)
    volatile = parser_helpers.is_volatile(type_desc)
    type_namespace, type_name = parser_helpers.get_namespace_and_type_name_from_type_desc(type_desc)
    if is_ioc_type is True:
        return f"{parser_helpers.get_full_qualified_name(type_namespace, type_name)}" \
               f"{' const' if const is True else ''}" \
               f"{' volatile' if volatile is True else ''}*"
    else:
        return f"{parser_helpers.get_full_qualified_name(type_namespace, type_name)}" \
               f"{' const' if const is True else ''}" \
               f"{' volatile' if volatile is True else ''}&"


def get_mangled_function_name(
        namespace: common_types.LexgineNamespace,
        parent_class: CppClass,
        using_directives: dict,
        using_dependency_list: set,
        function: Optional[CppMethod],
        kind: common_types.FunctionKind = common_types.FunctionKind.DEFAULT
) -> str:

    def process_namespace_definition(namespace_definition: str) -> str:
        namespace_definition_parts = namespace_definition.split(sep='::')
        return reduce(lambda x, y: x + y[0].upper() + y[1:],
                      namespace_definition_parts[1:],
                      namespace_definition_parts[0])

    def process_type(type_token: str) -> str:
        type_token = replace_fp_type_with_using_directive(
            type_token, parent_class, function, using_directives, using_dependency_list
        )
        rv = (type_token.replace(" ", "").replace('&', '_LVALREF_').replace('&&', '_RVALREF_').replace('*', "_PTR_")
              .replace('const', '_CONST_').replace('volatile', '_VOLATILE_'))
        rv = process_namespace_definition(rv)
        rv = rv.replace('<', "_TMPLB_").replace('>', "_TMPLE_").replace('__', '_')
        return rv

    namespace_token_part = process_namespace_definition(namespace.definition)
    class_token_part = parent_class["name"]
    return_type_part = ""

    if function is not None and kind == common_types.FunctionKind.DEFAULT:
        rtn_type = function["rtnType"]
        if (parser_helpers.is_primitive_type(rtn_type) is False
                and parser_helpers.is_pointer_type(rtn_type) is False
                and parser_helpers.is_reference_type(rtn_type) is False):
            return_type_part = f"_RTNTYPE_{process_type(rtn_type)}_LVALREF_"

    method_token_part = ""
    parameters_token_part = ""
    if (kind == common_types.FunctionKind.DEFAULT
            or kind == common_types.FunctionKind.CONSTRUCTOR) and function is not None:
        method_token_part = function["name"]
        parameters_token_part = reduce(
            lambda x, y: x + f"YY{process_type(y['type'])}",
            function["parameters"], "")

    # override method name section in mangling for constructors and destructors
    if kind == common_types.FunctionKind.CONSTRUCTOR:
        method_token_part = "CreateInstance"
    elif kind == common_types.FunctionKind.DESTRUCTOR:
        method_token_part = "DestroyInstance"

    name_string = f"{namespace_token_part}XXXX{class_token_part}XXXX{method_token_part}{parameters_token_part}" \
                  f"{return_type_part}"

    return name_string


def generate_export_function_definition(
        list_of_ioc_interfaces: List[common_types.ExportedResourceDescriptor],
        namespace: common_types.LexgineNamespace,
        parent_class: CppClass,
        function: Optional[Union[CppMethod, List[CppMethod]]],
        using_dependency_list: set
) -> str:
    functions_to_define: List[CppMethod] = []
    generate_construction_destruction_code = False
    if isinstance(function, list):
        functions_to_define = function
    elif isinstance(function, CppMethod):
        functions_to_define = [function]
    elif function is None:
        functions_to_define = [
            e for e in parent_class["methods"]["public"]
            if e["constructor"] is True and e["name"] == parent_class["name"]
            and e["deleted"] is False
        ]
        generate_construction_destruction_code = True

    using_dict = {v['type']: k for k, v in parent_class["using"].items()}
    referred_deleters = set()

    rv = ""
    for foo in functions_to_define:
        output_parameter_type_desc = None
        output_parameter_is_ioc = False
        parameters_string = "("
        if generate_construction_destruction_code is True:
            return_type_string = "void"
            name_string = get_mangled_function_name(
                namespace,
                parent_class,
                using_dict,
                using_dependency_list,
                foo,
                common_types.FunctionKind.CONSTRUCTOR
            )
            parameters_string += "void* p_destination"
        else:
            return_type_desc = foo["rtnType"]
            is_unique_ptr_to_ioc, ioc_ptr_type_desc = parser_helpers.is_unique_pointer_to_ioc(
                return_type_desc, list_of_ioc_interfaces, namespace
            )

            if is_unique_ptr_to_ioc:
                return_type_string = "void"
                output_parameter_type_desc = f"std::shared_ptr<{ioc_ptr_type_desc.parent_namespace.definition}" \
                                             f"::{ioc_ptr_type_desc.parsed_class['name']}>&"
            elif (parser_helpers.is_primitive_type(return_type_desc)
                  or parser_helpers.is_pointer_type(return_type_desc)
                  or parser_helpers.is_reference_type(return_type_desc)):
                return_type_string = return_type_desc
            else:
                output_parameter_is_ioc, _ = parser_helpers.is_ioc_type_desc(
                    list_of_ioc_interfaces, namespace, return_type_desc
                )
                return_type_string = "void"
                output_parameter_type_desc = get_compatible_type_desc_for_output_parameter(
                    return_type_desc, output_parameter_is_ioc
                )

            name_string = get_mangled_function_name(namespace, parent_class,
                                                    using_dict,
                                                    using_dependency_list, foo,
                                                    common_types.FunctionKind.DEFAULT)
            parameters_string += f"void {'const' if foo['const'] is True else ''}* p_instance"
            if output_parameter_type_desc is not None:
                parameters_string += f", {output_parameter_type_desc} " \
                                     f"{'p_' if output_parameter_is_ioc else ''}destination"
        if len(foo["parameters"]):
            parameters_string += ', '

        param_call_list = ""
        for idx, (is_last, param) in enumerate(util.signal_last(foo["parameters"])):
            param_type = param["type"]
            param_name = param["name"]
            if param_name[0] == '&' or param_name[0] == '*':
                param_type += param_name[0]
                param_name = f"param{idx}" if len(
                    param_name) == 1 else param_name[1:]

            parameter_type = replace_fp_type_with_using_directive(
                param_type, parent_class, foo, using_dict, using_dependency_list
            )
            parameters_string += f"{parameter_type} {param_name}"
            if parameter_type[-2:] != "&&":
                param_call_list += param_name
            else:
                param_call_list += f"std::move({param_name})"
            if is_last is False:
                parameters_string += ", "
                param_call_list += ", "
        parameters_string += ")"

        rv += f"LEXGINE_API {return_type_string} {name_string}{parameters_string}\n"
        if generate_construction_destruction_code is True:
            rv += "{\n\tnew (p_destination) " + f"{namespace.definition}::{parent_class['name']}" \
                  + "{" + param_call_list + "};\n}\n\n"
        else:
            if output_parameter_type_desc is None:
                rv += "{\n\t" + f"{'return' if return_type_string != 'void' else ''} reinterpret_cast<" \
                      + f"{namespace.definition}::{parent_class['name']} {'const' if foo['const'] else ''}*>" \
                        f"(p_instance)->{foo['name']}({param_call_list});\n" + "}\n\n"
            else:
                if output_parameter_is_ioc is False:
                    if is_unique_ptr_to_ioc:
                        referred_deleter_name = get_mangled_function_name(
                            ioc_ptr_type_desc.parent_namespace,
                            ioc_ptr_type_desc.parsed_class, using_dict,
                            using_dependency_list, None,
                            common_types.FunctionKind.DESTRUCTOR
                        ) + "__deleter"
                        referred_deleters.add(f"void {referred_deleter_name}(void*);\n")
                        rv += "{\n" \
                              f"\tauto _temp = reinterpret_cast<" \
                              f"{namespace.definition}::{parent_class['name']} {'const' if foo['const'] else ''}*>" \
                              f"(p_instance)" \
                              f"->{foo['name']}({param_call_list});\n" \
                              f"\tstd::unique_ptr<" \
                              f"{ioc_ptr_type_desc.parent_namespace.definition}" \
                              f"::{ioc_ptr_type_desc.parsed_class['name']}, void(*)(void*)> _temp1" \
                              "{_temp.release(), " + f"&{referred_deleter_name}" + "};\n" \
                              f"\tdestination = std::shared_ptr<" \
                              f"{ioc_ptr_type_desc.parent_namespace.definition}" \
                              f"::{ioc_ptr_type_desc.parsed_class['name']}>" \
                              "{std::move(_temp1)};\n}\n\n"
                    else:
                        rv += "{\n\tdestination = reinterpret_cast<" \
                              + f"{namespace.definition}::{parent_class['name']} " \
                                f"{'const' if foo['const'] is True else ''}*>(p_instance)"\
                                f"->{foo['name']}({param_call_list});\n" + "}\n\n"
                else:
                    rv += "{\n\t" + f"new (p_destination) {return_type_desc}" \
                          + "{std::move(reinterpret_cast<" \
                          + f"{namespace.definition}::{parent_class['name']}*>(p_instance)->{foo['name']}" \
                            f"({param_call_list})" + "};\n}\n\n"

    rv = reduce(lambda x, y: f"{x}{y}", referred_deleters, "") + "\n\n" + rv

    if generate_construction_destruction_code is True:
        # Generate destruction code
        destructor_name_string = get_mangled_function_name(
            namespace, parent_class, using_dict, using_dependency_list, None, common_types.FunctionKind.DESTRUCTOR
        )
        rv += f"LEXGINE_API void {destructor_name_string}(void* p_instance)\n" \
              + "{\n" \
                f"\treinterpret_cast<{namespace.definition}::{parent_class['name']}*>(p_instance)" \
                f"->~{parent_class['name']}();\n" \
                "}\n\n"
        rv += f"void {destructor_name_string}__deleter(void* p_instance)\n" \
              + "{\n" \
                f"\tdelete reinterpret_cast<{namespace.definition}::{parent_class['name']}*>(p_instance);\n" \
                "}\n\n"

    return rv


def create_export_interface_imported_opaque_class(
        list_of_ioc_interfaces: List[common_types.ExportedResourceDescriptor],
        resource: common_types.ExportedResourceDescriptor,
        config: dict
) -> common_types.LexgineDllExportInterface:
    template_path: Path = Path(get_template_descriptor(config, common_types.InterfaceKind.EXPORT))
    with open(template_path.as_posix(), 'r') as f:
        source_template_string = global_names.BSD_3CLAUSE_LICENSE + f.read()

    all_resources_raveled = resource.ravel()
    cpp_exporting_source = ""
    for r in all_resources_raveled:
        if r.is_ioc:
            exported_flags_using_declarations = ""
            exported_enums = ""
            exported_unions = ""
            exported_nested_classes = ""
            referred_fp_using_declarations = set()

            for flags in r.exported_flags:
                exported_flags_using_declarations += f"using {flags.name} = {r.parent_namespace.definition}::" \
                                                     f"{r.parsed_class['name']}::{flags.name};\n"

            if r.exported_enums is not None:
                for e in r.exported_enums:
                    exported_enums += f"using {e['name']} = {r.parent_namespace.definition}::" \
                                      f"{r.parsed_class['name']}::{e['name']};\n"

            if r.exported_unions is not None:
                for e in r.exported_unions:
                    exported_unions += f"using {e['name']} = {r.parent_namespace.definition}::" \
                                       f"{r.parsed_class['name']}::{e['name']};\n"

            if r.nested_resources is not None:
                for nr in r.nested_resources:
                    exported_nested_classes += f"using {nr.parsed_class['name']} = {nr.parent_namespace.definition}" \
                                               f"::{nr.parsed_class['name']};\n"

            constructors_and_destructors_definitions = generate_export_function_definition(
                list_of_ioc_interfaces, r.parent_namespace, r.parsed_class, None, referred_fp_using_declarations
            )
            parsed_methods_definitions = generate_export_function_definition(
                list_of_ioc_interfaces, r.parent_namespace, r.parsed_class, r.parsed_methods,
                referred_fp_using_declarations
            )

            source_body = f"\n\n{constructors_and_destructors_definitions}\n\n{parsed_methods_definitions}"
            fp_using_declarations = reduce(lambda x, y: f"{x}using {y[0]} = {y[1]};\n",
                                           referred_fp_using_declarations, "")

            cpp_exporting_source += Template(source_template_string).substitute(
                year=datetime.date.today().year,
                exporting_header=r.exporting_header_path.as_posix(),
                exporting_namespace=r.parent_namespace.definition,
                flags_using_declarations=exported_flags_using_declarations,
                enums_using_declarations=exported_enums,
                unions_using_declarations=exported_unions,
                nested_classes_using_declarations=exported_nested_classes,
                fp_using_declarations=fp_using_declarations,
                source_body=source_body
            )
    return common_types.LexgineDllExportInterface(cpp_exporting_source)


def get_inheritance_mapped_type(
        context_namespace: common_types.LexgineNamespace,
        probed_type_desc: str,
        inherited_types_mapping: Dict[str, dict]
) -> Optional[str]:
    namespace_context_nesting_variants, type_name = \
        parser_helpers.get_context_nested_namespace_variations_for_type_desc(context_namespace, probed_type_desc)
    rv = None
    for e in namespace_context_nesting_variants:
        probed_fully_qualified_type = f"{e}::{type_name}"
        if probed_fully_qualified_type in inherited_types_mapping:
            rv = inherited_types_mapping[probed_fully_qualified_type][
                "mapped_object_name"]
            break
    return rv


def create_import_interface_imported_opaque_class(
        list_of_ioc_interfaces: List[common_types.ExportedResourceDescriptor],
        resource: common_types.ExportedResourceDescriptor,
        inherited_types_mapping: Dict[str, dict],
        config: dict
) -> common_types.LexgineRuntimeLinkInterface:
    template_desc = get_template_descriptor(config, common_types.InterfaceKind.IMPORT)
    import_template_cpp_path: Optional[Path] = Path(template_desc["source"])
    import_template_hpp_path: Optional[Path] = Path(template_desc["header"])
    import_template_helper_path: Optional[Path] = Path(template_desc["helper"])
    with open(import_template_cpp_path.as_posix(), 'r') as f:
        import_template_cpp_string = global_names.BSD_3CLAUSE_LICENSE + f.read()
    with open(import_template_hpp_path.as_posix(), 'r') as f:
        import_template_hpp_string = global_names.BSD_3CLAUSE_LICENSE + f.read()
    with open(import_template_helper_path.as_posix(), 'r') as f:
        import_template_helper_string = f.read()

    includes_list_src: str = reduce(lambda x, y: x + f"#include <{y}>\n", resource.includes, "")

    def create_hpp_cpp_sources(r: common_types.ExportedResourceDescriptor) -> Tuple[str, str]:

        def extract_structure_definition_from_source(s: Union[CppEnum, CppUnion, CppClass]) -> str:
            line_number_zb = s["line_number"] - 1
            begin_idx = util.get_line_offset(r.exporting_header_source, line_number_zb)
            _, end_idx = util.extract_scope(r.exporting_header_source, begin_idx, common_types.ScopeType.BRACES)
            return r.exporting_header_source[begin_idx:end_idx + 1]

        if r.is_ioc:
            ioc_base_name = "lexgine::api::Ioc"

            inherited_classes: Dict[str, str] = {}
            inheritance_remapping_mask: Dict[str, bool] = {}
            for inherited_type in r.parsed_class['inherits']:
                target_name = get_inheritance_mapped_type(
                    r.parent_namespace, inherited_type["decl_name"], inherited_types_mapping
                )
                if target_name is None:
                    target_name = inherited_type['decl_name']
                    inheritance_remapping_mask[target_name] = False
                else:
                    inheritance_remapping_mask[target_name] = True
                inherited_classes[target_name] = inherited_type["access"]
            if len(inherited_classes) == 0:
                inherited_classes[ioc_base_name] = "virtual public"

            inheritance_list: str = reduce(lambda x, y: f"{x}{y[1][1]} {y[1][0]}{', ' if y[0] is False else ''}",
                                           util.signal_last(inherited_classes.items()), ": ")

            class IocInitStrategy(Enum):
                API = 0
                RUNTIME_RAW_PTR = 1
                RUNTIME_SHARED_PTR = 2
                FAKE_CONSTRUCTION_TAG = 3

            def get_base_classes_init_list(
                    ioc_init_strategy: IocInitStrategy = IocInitStrategy.API,
                    argument_name: Optional[str] = None
            ) -> str:
                if ioc_init_strategy == IocInitStrategy.API:
                    ioc_enum_name = f"{r.parent_namespace.definition.replace('::', '_').upper()}" \
                                    f"_{util.convert_camel_case_to_snake_case(r.short_name).upper()}"
                    base_classes_init_list = "\n\t: Ioc{" \
                                             f"lexgine::common::ImportedOpaqueClass::{ioc_enum_name}, " \
                                             f"api__{destructor_mangled_name}" \
                                             "}"
                elif (ioc_init_strategy == IocInitStrategy.RUNTIME_RAW_PTR
                      or ioc_init_strategy == IocInitStrategy.RUNTIME_SHARED_PTR):
                    base_classes_init_list = "\n\t: Ioc{" + argument_name + "}"
                elif ioc_init_strategy == IocInitStrategy.FAKE_CONSTRUCTION_TAG:
                    base_classes_init_list = "\n\t: Ioc{lexgine::api::FakeConstruction_tag{}}"
                else:
                    raise AssertionError("Unknown initialization strategy of lexgine::api::Ioc class requested")

                for base_class_name in inherited_classes.keys():
                    if base_class_name != ioc_base_name:
                        base_classes_init_list += f"\n\t, {base_class_name}" \
                                                  + ("{lexgine::api::FakeConstruction_tag{}}"
                                                     if inheritance_remapping_mask[base_class_name] is False else "{}")
                return base_classes_init_list

            supported_public_properties_list: List[CppVariable] = []
            for p in r.parsed_class["properties"]["public"]:
                if p["static"] is True:
                    print(f"WARNING: property {p['name']} is static. Static properties are not supported in "
                          f"IOC export types.")
                    continue
                supported_public_properties_list.append(p)

            property_accessor_dummy_class_definition = \
                f"struct {r.parsed_class['name']}Dummy final\n" + "{\n" + reduce(
                    lambda x, y: x + f"\t{y['type']} {'const' if y['constant'] else ''} {y['name']};\n",
                    supported_public_properties_list,
                    ""
                ) + "};\n"

            public_properties_setters_getters_declarations = ""
            public_properties_setters_getters_definitions = ""
            for p in supported_public_properties_list:
                name = util.convert_snake_case_to_camel_case(p["name"])
                name = name[0].upper() + name[1:]
                public_properties_setters_getters_declarations += f"\t{p['type']} get{name}() const;\n"
                public_properties_setters_getters_definitions += \
                    f"{p['type']} {r.parent_namespace.definition}::{r.parsed_class['name']}::get{name}() const\n" + \
                    '{\n\t' + f"return *reinterpret_cast<{p['type']} const*>" \
                              f"(static_cast<uint8_t const*>(Ioc::getNative()) " \
                              f"+ offsetof({r.parsed_class['name']}Dummy, {p['name']}));\n" + '}\n'

                if not p["constant"]:
                    public_properties_setters_getters_declarations += \
                        f"\tvoid set{name}(lexgine::api::public_property_type_accessor<{p['type']}>" \
                        f"::value_type value);\n"
                    public_properties_setters_getters_definitions \
                        += f"void {r.parent_namespace.definition}::{r.parsed_class['name']}::set{name}(" \
                           f"lexgine::api::public_property_type_accessor<{p['type']}>::value_type value)\n" \
                           + '{\n\t' + f"*reinterpret_cast<{p['type']}*>(static_cast<uint8_t*>(Ioc::getNative()) " \
                                       f"+ offsetof({r.parsed_class['name']}Dummy, {p['name']})) = value;\n" + '}\n'

            api_methods_declarations = ""
            api_methods_definitions = ""
            api_linked_pointers = ""
            link_methods_call_list = "\t"
            constructors = [
                m for m in r.parsed_class["methods"]["public"]
                if m["constructor"] is True and m["name"] ==
                r.parsed_class["name"] and m["deleted"] is False
            ]

            flags_using_declarations = ""
            referred_fp_using_declarations = set()
            exported_enums_definitions = ""
            exported_enums_using_declarations = ""
            exported_unions_definitions = ""
            exported_unions_using_declarations = ""

            for f in r.exported_flags:
                flags_using_declarations \
                    += f"using {f.name} = {r.parent_namespace.definition}::{r.parsed_class['name']}::{f.name};\n"

            if r.exported_enums is not None:
                for enum in r.exported_enums:
                    exported_enums_definitions += f"{extract_structure_definition_from_source(enum)};\n\n"
                    exported_enums_using_declarations \
                        += f"using {enum['name']} = {r.parent_namespace.definition}" \
                           f"::{r.parsed_class['name']}::{enum['name']};\n"

            if r.exported_unions is not None:
                for union in r.exported_unions:
                    exported_unions_definitions += f"{extract_structure_definition_from_source(union)};\n\n"
                    exported_unions_using_declarations \
                        += f"using {union['name']} = {r.parent_namespace.definition}" \
                           f"::{r.parsed_class['name']}::{union['name']};\n"

            using_dict = {
                v["type"]: k
                for k, v in r.parsed_class["using"].items()
            }
            for m in r.parsed_methods + constructors:
                compatible_rtn_type = m[
                    "rtnType"] if m['constructor'] is False else ''
                api_function_rtn_type = compatible_rtn_type
                rtn_type_is_object = False
                rtn_type_is_ioc = False
                rtn_type_resource_descriptor = None
                api_function_output_parameter_rtn_type = None

                rtn_type_is_unique_ptr_to_ioc, ioc_ptr_type_desc = parser_helpers.is_unique_pointer_to_ioc(
                    compatible_rtn_type,
                    list_of_ioc_interfaces,
                    r.parent_namespace
                )
                rtn_type_is_ptr_to_ioc = False
                if rtn_type_is_unique_ptr_to_ioc is False:
                    rtn_type_is_ptr_to_ioc, ioc_ptr_type_desc = parser_helpers.is_raw_pointer_to_ioc(
                        compatible_rtn_type,
                        list_of_ioc_interfaces,
                        r.parent_namespace
                    )
                if rtn_type_is_unique_ptr_to_ioc or rtn_type_is_ptr_to_ioc:
                    compatible_rtn_type = f"{ioc_ptr_type_desc.parent_namespace.definition}" \
                                          f"::{ioc_ptr_type_desc.parsed_class['name']}"

                api_methods_declarations += \
                    f"\t{compatible_rtn_type}{' ' if len(compatible_rtn_type) else ''}{m['name']}("
                api_methods_definitions += \
                    f"{compatible_rtn_type}{' ' if len(compatible_rtn_type) else ''}{r.parent_namespace.definition}" \
                    f"::{r.parsed_class['name']}::{m['name']}("
                call_list = ""
                mangled_method_name = get_mangled_function_name(
                    r.parent_namespace,
                    r.parsed_class,
                    using_dict,
                    referred_fp_using_declarations,
                    m,
                    common_types.FunctionKind.DEFAULT
                    if m["constructor"] is False else common_types.FunctionKind.CONSTRUCTOR
                )
                destructor_mangled_name = get_mangled_function_name(
                    r.parent_namespace,
                    r.parsed_class,
                    using_dict,
                    referred_fp_using_declarations,
                    None,
                    common_types.FunctionKind.DESTRUCTOR
                )
                link_methods_call_list += f'api__{mangled_method_name} = ' \
                                          f'reinterpret_cast<decltype(api__{mangled_method_name})>' \
                                          f'(linker.attemptLink("{mangled_method_name}"));\n\t'

                if len(compatible_rtn_type) == 0:  # the function is a constructor
                    compatible_rtn_type = "void"
                    api_function_rtn_type = "void"
                elif rtn_type_is_unique_ptr_to_ioc:
                    rtn_type_is_object = True
                    api_function_rtn_type = "void"
                    api_function_output_parameter_rtn_type = \
                        f"std::shared_ptr<{ioc_ptr_type_desc.parent_namespace.definition}" \
                        f"::{ioc_ptr_type_desc.parsed_class['name']}>&"
                elif (parser_helpers.is_primitive_type(compatible_rtn_type) is False
                      and parser_helpers.is_pointer_type(compatible_rtn_type) is False
                      and parser_helpers.is_reference_type(compatible_rtn_type) is False
                      and rtn_type_is_ptr_to_ioc is False):
                    rtn_type_is_object = True
                    rtn_type_is_ioc, rtn_type_resource_descriptor = parser_helpers.is_ioc_type_desc(
                        list_of_ioc_interfaces, r.parent_namespace, compatible_rtn_type
                    )
                    api_function_rtn_type = "void"
                    api_function_output_parameter_rtn_type = get_compatible_type_desc_for_output_parameter(
                        compatible_rtn_type, rtn_type_is_ioc
                    )

                api_pointer_declaration = f"{api_function_rtn_type}(LEXGINE_CALL* api__{mangled_method_name})" \
                                          f"(void {'const' if m['const'] is True else ''}*"
                if rtn_type_is_object:
                    api_pointer_declaration += f", {api_function_output_parameter_rtn_type}"

                nz_params = len(m["parameters"]) > 0
                if nz_params:
                    api_pointer_declaration += ", "

                for idx, (s, p) in enumerate(util.signal_last(m["parameters"])):
                    param_type = p["type"]
                    param_name = p["name"]
                    if param_name[0] == '&' or param_name[0] == '*':
                        param_type += param_name[0]
                        if len(param_name) == 1:
                            param_name = f"param{idx}" if len(param_name) == 1 else param_name[1:]

                    processed_type = replace_fp_type_with_using_directive(
                        param_type, r.parsed_class, m, using_dict, referred_fp_using_declarations
                    )
                    next_named_function_param = f"{processed_type} {param_name}"
                    api_methods_declarations += next_named_function_param
                    api_methods_definitions += next_named_function_param

                    if 'default' in p:
                        api_methods_declarations += f" = {p['default']}"
                    if s is False:
                        api_methods_declarations += ", "
                        api_methods_definitions += ", "

                    api_pointer_declaration += f"{processed_type}{', ' if s is False else ''}"
                    call_list += f"lexgine::api::unfold(" \
                                 f"{param_name if param_type[-2:] != '&&' else 'std::move(' + param_name + ')'})" \
                                 f"{', ' if s is False else ''}"
                api_methods_declarations += ')'
                api_methods_definitions += ')'
                api_pointer_declaration += ')'

                if m["const"] is True:
                    api_methods_declarations += " const"
                    api_methods_definitions += " const"

                api_linked_pointers += f"{api_pointer_declaration} = nullptr;\n"

                if 'doxygen' in m:
                    api_methods_declarations += f";    {m['doxygen']}\n"
                else:
                    api_methods_declarations += ";\n"

                if m["constructor"] is True:
                    api_methods_definitions += \
                        get_base_classes_init_list() \
                        + "\n{\n\t" + f"api__{mangled_method_name}" \
                                      f"(getNative(){', ' + call_list if nz_params else ''});\n" + "}\n\n"
                elif rtn_type_is_object is True:
                    if rtn_type_is_ioc is True:
                        ioc_enum_name = \
                            f"{rtn_type_resource_descriptor.parent_namespace.definition.replace('::', '_').upper()}_" \
                            f"{util.convert_camel_case_to_snake_case(rtn_type_resource_descriptor.parsed_class['name']).upper()}"
                        api_methods_definitions += \
                            "\n{\n\t" + f"{compatible_rtn_type} rv" \
                            + "{" + f"lexgine::common::ImportedOpaqueClass::{ioc_enum_name}" + "};\n\t" \
                            + f"api__{mangled_method_name}" \
                              f"(getNative(), rv.getNative(){', ' + call_list if nz_params else ''});\n\t" \
                              "return rv;\n}\n\n"
                    else:
                        if rtn_type_is_unique_ptr_to_ioc:
                            api_methods_definitions += \
                                "\n{\n\t" + f"{api_function_output_parameter_rtn_type.replace('&', '')} rv" \
                                + "{};\n\t" \
                                  f"api__{mangled_method_name}" \
                                  f"(getNative(), rv{', ' + call_list if nz_params else ''});\n\t" \
                                  f"return {compatible_rtn_type}" + "{rv};\n}\n\n"
                        else:
                            api_methods_definitions += \
                                "\n{\n\t" + f"{compatible_rtn_type} rv" \
                                + "{};\n\t" \
                                  f"api__{mangled_method_name}" \
                                  f"(getNative(), rv{', ' + call_list if nz_params else ''});\n\t" \
                                  "return rv;\n}\n\n"
                else:
                    if rtn_type_is_ptr_to_ioc:
                        api_methods_definitions += \
                            "\n{\n\tvoid* ptr = api__" \
                            f"{mangled_method_name}(getNative(){', ' + call_list if nz_params else ''});\n\t" \
                            f"return {ioc_ptr_type_desc.parent_namespace.definition}" \
                            f"::{ioc_ptr_type_desc.parsed_class['name']}" \
                            "{static_cast<lexgine::api::Ioc*>(ptr)};\n" \
                            "}\n\n"
                    else:
                        api_methods_definitions += \
                            "\n{\n\t" \
                            f"{'return ' if m['rtnType'] != 'void' else ''}" \
                            f"api__{mangled_method_name}(getNative(){', ' + call_list if nz_params else ''});\n" \
                            '}\n\n'

            # Link default constructor if no custom constructors were provided
            if len(constructors) == 0:
                api_methods_declarations += f"\t{r.parsed_class['name']}();\n"
                api_methods_definitions \
                    += f"{r.parent_namespace.definition}::{r.parsed_class['name']}" \
                       f"::{r.parsed_class['name']}(){get_base_classes_init_list()}\n" + "{}\n\n"

            # Add construction from IOC pointers
            api_methods_declarations += f"\t{r.parsed_class['name']}(std::shared_ptr<lexgine::api::Ioc> const& ptr);\n"
            api_methods_definitions += \
                f"{r.parent_namespace.definition}::{r.parsed_class['name']}::{r.parsed_class['name']}" \
                f"(std::shared_ptr<lexgine::api::Ioc> const& ptr)" \
                f"{get_base_classes_init_list(IocInitStrategy.RUNTIME_SHARED_PTR, 'ptr')}\n" \
                "{}\n\n"
            api_methods_declarations += f"\t{r.parsed_class['name']}(lexgine::api::Ioc* ptr);\n"
            api_methods_definitions += \
                f"{r.parent_namespace.definition}::{r.parsed_class['name']}::{r.parsed_class['name']}" \
                f"(lexgine::api::Ioc* ptr){get_base_classes_init_list(IocInitStrategy.RUNTIME_RAW_PTR, 'ptr')}\n" \
                "{}\n\n"

            # Link destructors
            protected_apis = ""
            api_linked_pointers += f"static void(LEXGINE_CALL* api__{destructor_mangled_name})(void*) = nullptr;\n"
            link_methods_call_list += f'api__{destructor_mangled_name} = ' \
                                      f'reinterpret_cast<decltype(api__{destructor_mangled_name})>' \
                                      f'(linker.attemptLink("{destructor_mangled_name}"));\n'
            api_methods_declarations += f"\t~{r.parsed_class['name']}() = default;"

            if r.is_inherited:
                protected_apis += f"\t{r.parsed_class['name']}(lexgine::api::FakeConstruction_tag);"
                api_methods_definitions += f"\n\n{r.parent_namespace.definition}::{r.parsed_class['name']}::" \
                                           f"{r.parsed_class['name']}(lexgine::api::FakeConstruction_tag)" \
                                           f"{get_base_classes_init_list(IocInitStrategy.FAKE_CONSTRUCTION_TAG)}" \
                                           + "{}\n\n"

            fp_using_declarations_hpp = reduce(lambda x, y: f"{x}using {y[0]} = {y[1]};\n\t",
                                               referred_fp_using_declarations, "\t")
            fp_using_declarations_cpp = reduce(lambda x, y: f"{x}using {y[0]} = {r.parsed_class['name']}::{y[0]};\n",
                                               referred_fp_using_declarations, "")

            class_qualifier = f"{'final' if resource.parsed_class['final'] is True else ''}"
            exported_flags = reduce(lambda x, y: f"{x}\t{y.declaration_string};\n", resource.exported_flags, "")

            nested_classes_forward_declarations = "\t"
            nested_classes_declarations = ""
            nested_classes_using_declarations = ""

            import_api_cpp = ""
            if r.nested_resources is not None:
                for nested_r in r.nested_resources:
                    nested_classes_forward_declarations += f"{nested_r.token_type.value} " \
                                                           f"{nested_r.parsed_class['name']};\n\t"
                    nested_classes_using_declarations += f"using {nested_r.parsed_class['name']}" \
                                                         f" = {nested_r.parent_namespace.definition}" \
                                                         f"::{nested_r.parsed_class['name']};\n"
                    hpp, cpp = create_hpp_cpp_sources(nested_r)
                    nested_classes_declarations += f"{hpp}\n\n"
                    import_api_cpp += cpp

            class_declaration = Template(import_template_helper_string).substitute(
                    class_token=resource.token_type.value,
                    class_name=resource.parsed_class["name"],
                    inheritance_list=inheritance_list,
                    class_qualifier=class_qualifier,
                    public_properties_setters_and_getters_declarations=public_properties_setters_getters_declarations,
                    exported_flags=exported_flags,
                    public_unions=exported_unions_definitions,
                    public_enums=exported_enums_definitions,
                    fp_using_declarations=fp_using_declarations_hpp,
                    nested_classes_forward_declarations=nested_classes_forward_declarations,
                    nested_classes=nested_classes_declarations,
                    api_methods=api_methods_declarations,
                    protected_apis=protected_apis
            )

            import_api_cpp += Template(import_template_cpp_string).substitute(
                year=datetime.date.today().year,
                api_name=f"{util.convert_camel_case_to_snake_case(resource.parsed_class['name'])}",
                namespace=resource.parent_namespace.definition,
                public_properties_memory_layout_declaration=property_accessor_dummy_class_definition,
                flags_using_declarations=flags_using_declarations,
                unions_using_declarations=exported_unions_using_declarations,
                enums_using_declarations=exported_enums_using_declarations,
                nested_classes_using_declarations=nested_classes_using_declarations,
                fp_using_declarations=fp_using_declarations_cpp,
                api_function_pointers_declarations=api_linked_pointers,
                class_name=resource.parsed_class["name"],
                link_methods_call_list=link_methods_call_list,
                public_properties_setters_getters_definitions=public_properties_setters_getters_definitions,
                api_methods_definitions=api_methods_definitions
            )
        else:
            return f"{extract_structure_definition_from_source(r.parsed_class)};", ""
        return class_declaration, import_api_cpp

    class_declaration, import_api_cpp = create_hpp_cpp_sources(resource)
    import_api_hpp = Template(import_template_hpp_string).substitute(
        year=datetime.date.today().year,
        hpp_guard=create_hpp_guard(resource.parent_namespace, resource.parsed_class),
        namespace=resource.parent_namespace.definition,
        includes_list=includes_list_src,
        class_declaration=class_declaration
    )
    return common_types.LexgineRuntimeLinkInterface(source=import_api_cpp, header=import_api_hpp)



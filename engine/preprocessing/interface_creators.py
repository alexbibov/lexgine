import datetime
import git
from typing import List
from pathlib import Path
from functools import reduce
from string import Template

import global_names
import common_types
import util
from template_helpers import fetch_import_templates, get_template_descriptor, create_hpp_guard
from interface_ioc import create_export_interface_imported_opaque_class, create_import_interface_imported_opaque_class


create_export_interface_imported_opaque_class = create_export_interface_imported_opaque_class
create_import_interface_imported_opaque_class = create_import_interface_imported_opaque_class


def create_ioc_enum_clause(resource: common_types.ExportedResourceDescriptor) -> str:
    assert resource.is_ioc is True
    return f"{resource.parent_namespace.definition.replace('::', '_').upper()}" \
           f"_{util.convert_camel_case_to_snake_case(resource.short_name).upper()}"


def create_import_interface_data_class(
        resource: common_types.ExportedResourceDescriptor,
        config: dict
) -> common_types.LexgineRuntimeLinkInterface:
    assert resource.token_type == common_types.TokenType.CLASS or resource.token_type == common_types.TokenType.STRUCT
    _, import_template_hpp_string = fetch_import_templates(get_template_descriptor(config, common_types.InterfaceKind.IMPORT))

    includes_list = reduce(lambda x, y: f"{x}#include <{y.as_posix()}>\n", resource.includes, "")

    definition_begin_offset = util.get_line_offset(
        resource.exporting_header_source,
        resource.parsed_class["line_number"] - 1
    )
    definition_begin_offset = resource.exporting_header_source.find(
        str(resource.token_type.value), definition_begin_offset
    ) + len(resource.token_type.value)
    _, definition_end_offset = util.extract_scope(
        resource.exporting_header_source, definition_begin_offset, common_types.ScopeType.BRACES
    )

    definition_str = \
        resource.exporting_header_source[definition_begin_offset:definition_end_offset + 1].strip(' \t\n\r')
    header = Template(import_template_hpp_string).substitute(
        year=datetime.date.today().year,
        hpp_guard=create_hpp_guard(resource.parent_namespace, resource.parsed_class),
        includes_list=includes_list,
        parent_namespace=resource.parent_namespace.definition,
        token_type=resource.token_type.value,
        data_class_definition=definition_str
    )
    return common_types.LexgineRuntimeLinkInterface(None, header)


def create_export_interface_imported_opaque_class_traits(
        list_of_ioc_interfaces: List[common_types.ExportedResourceDescriptor],
        config: dict
) -> common_types.LexgineDllExportInterface:
    template_path: Path = Path(get_template_descriptor(config, common_types.InterfaceKind.EXPORT))
    with open(template_path.as_posix(), 'r') as f:
        source_template_string = global_names.BSD_3CLAUSE_LICENSE + f.read()
    includes_list = ""
    switch_clauses = ""
    for resource in list_of_ioc_interfaces:
        switch_clause_name = create_ioc_enum_clause(resource)
        switch_clauses += f"\t\tcase lexgine::common::ImportedOpaqueClass::{switch_clause_name}: " \
                          f"return sizeof({resource.parent_namespace.definition}::{resource.parsed_class['name']});\n"
        if resource.parent_resource is None:
            includes_list += f"#include <{resource.exporting_header_path.as_posix()}>\n"

    cpp_exporting_source = Template(source_template_string).substitute(
        year=datetime.date.today().year,
        includes_list=includes_list,
        ioc_name_switch_clauses=switch_clauses
    )
    return common_types.LexgineDllExportInterface(source=cpp_exporting_source)


def create_import_interface_imported_opaque_class_traits(
        list_of_ioc_interfaces: List[common_types.ExportedResourceDescriptor],
        config: dict
) -> common_types.LexgineRuntimeLinkInterface:
    _, import_template_hpp_string = fetch_import_templates(get_template_descriptor(config, common_types.InterfaceKind.IMPORT))
    ioc_names_list = ""
    for resource in list_of_ioc_interfaces:
        ioc_names_list += f"\t{create_ioc_enum_clause(resource)},\n"
    ioc_names_list += "\tCOUNT"
    hpp_api_source = Template(import_template_hpp_string).substitute(
        year=datetime.date.today().year,
        ioc_names_list=ioc_names_list
    )
    return common_types.LexgineRuntimeLinkInterface(source=None, header=hpp_api_source)


def create_import_interface_api_linker(
        list_of_ioc_interfaces: List[common_types.ExportedResourceDescriptor],
        config: dict
) -> common_types.LexgineRuntimeLinkInterface:
    import_templates_desc: dict = get_template_descriptor(config, common_types.InterfaceKind.IMPORT)
    import_template_cpp_string, import_template_hpp_string = fetch_import_templates(
        import_templates_desc)

    includes_list = reduce(
        lambda x, y: f"{x}#include <{y.runtime_api_header_path.as_posix()}>\n", list_of_ioc_interfaces, ""
    )
    link_calls_list = "\t"
    for resource in list_of_ioc_interfaces:
        fully_qualified_name = f"{resource.parent_namespace.definition}::{resource.parsed_class['name']}"
        link_calls_list += 'rv.emplace(std::make_pair(std::string{"' + fully_qualified_name + '"}, ' \
                           + f"{fully_qualified_name}::link(module)));\n\t"

    import_template_hpp_string = Template(import_template_hpp_string).substitute(year=datetime.date.today().year)
    import_template_cpp_string = Template(import_template_cpp_string).substitute(
        year=datetime.date.today().year,
        includes_list=includes_list,
        link_calls_list=link_calls_list
    )
    return common_types.LexgineRuntimeLinkInterface(source=import_template_cpp_string, header=import_template_hpp_string)


def create_import_interface_enumeration(
        resource: common_types.ExportedResourceDescriptor,
        config: dict
) -> common_types.LexgineRuntimeLinkInterface:
    import_templates_desc: dict = get_template_descriptor(config, common_types.InterfaceKind.IMPORT)
    _, import_template_hpp_string = fetch_import_templates(import_templates_desc)
    includes_list = reduce(lambda x, y: f"{x}#include <{y.as_posix()}>\n", resource.includes, "")
    namespace_name = resource.parent_namespace.definition

    enum_definition_begin = util.get_line_offset(
        resource.exporting_header_source,
        resource.parsed_class["line_number"] - 1
    )
    _, enum_definition_end = util.extract_scope(
        resource.exporting_header_source,
        enum_definition_begin,
        common_types.ScopeType.BRACES
    )
    enum_definition = resource.exporting_header_source[enum_definition_begin:enum_definition_end + 1]

    import_template_hpp_string = Template(import_template_hpp_string).substitute(
        year=datetime.date.today().year,
        hpp_guard=create_hpp_guard(resource.parent_namespace, resource.parsed_class),
        includes_list=includes_list,
        namespace_name=namespace_name,
        enum_definition=enum_definition
    )
    return common_types.LexgineRuntimeLinkInterface(source=None, header=import_template_hpp_string)


def create_import_interface_flags(
        resource: common_types.ExportedResourceDescriptor,
        config: dict
) -> common_types.LexgineRuntimeLinkInterface:
    import_templates_desc: dict = get_template_descriptor(config, common_types.InterfaceKind.IMPORT)
    _, import_template_hpp_string = fetch_import_templates(import_templates_desc)
    includes_list = reduce(lambda x, y: f"{x}#include <{y.as_posix()}>\n", resource.includes, "")

    import_template_hpp_string = Template(import_template_hpp_string).substitute(
        year=datetime.date.today().year,
        includes_list=includes_list,
        parent_namespace=resource.parent_namespace.definition,
        flags_definition=resource.parsed_class.declaration_string
    )
    return common_types.LexgineRuntimeLinkInterface(source=None, header=import_template_hpp_string)


def create_import_interface_build_info(
        source_tree_path: Path,
        config: dict
) -> common_types.LexgineRuntimeLinkInterface:
    repo = git.Repo(source_tree_path, search_parent_directories=True)
    sha = repo.head.commit.hexsha
    import_templates_desc: dict = get_template_descriptor(config, common_types.InterfaceKind.IMPORT)
    _, import_template_hpp_string = fetch_import_templates(import_templates_desc)

    version_path: Path = source_tree_path / "version"
    if version_path.exists() is False or version_path.is_file() is False:
        raise RuntimeError("Unable to find 'version' file in root directory of engine source tree. The repository "
                           "might be corrupted")
    with open(version_path.as_posix(), 'r') as f:
        version = f.read()
        version_elements = version.split(sep='.')
        version = 0
        for is_last, e in util.signal_last(version_elements):
            version ^= int(e)
            if not is_last:
                version = version << 16

    import_template_hpp_string = Template(import_template_hpp_string).substitute(
        year=datetime.date.today().year,
        project_version=f'{version}ui64',
        git_commit_hash=f'"{sha}"'
    )
    return common_types.LexgineRuntimeLinkInterface(source=None, header=import_template_hpp_string)

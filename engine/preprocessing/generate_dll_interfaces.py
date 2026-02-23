from __future__ import annotations
import argparse
import json
import shutil
from pathlib import Path
from CppHeaderParser import CppHeader, CppParseError, CppClass, CppEnum, CppMethod, CppVariable, CppUnion
from typing import List, Optional, Tuple, Dict, Set, Iterable, Union

import global_names
import common_types
import util
import lexgine_utils
import parser_helpers
import token_fetch_functions
import interface_creators as ic
from interface_ioc import get_inheritance_mapped_type


def map_inherited_type(type: str, replaced_types_dictionary: dict) -> Optional[str]:
    for k in replaced_types_dictionary:
        if util.is_subname(type, k):
            return replaced_types_dictionary[k]
    return None


def main(arguments: argparse.Namespace):
    print(arguments.headers)
    with open(arguments.config, 'r') as f:
        configuration = json.load(f)
    headers = arguments.headers.split(sep=";")

    ignored_hpp_paths: List[Path] = []
    for hpp in configuration.get("ignored_headers", []):
        hpp = str(hpp).strip(' \t\r\n')
        if not hpp.startswith("engine"):
            print(
                f'WARNING: the path {hpp} listed among ignored include directories in the configuration file '
                f'{arguments.config} does not start with "engine". This path will not be included into the list of '
                f'ignored engine directories as it is impossible to relate it to the engine directory structure'
            )
        else:
            ignored_hpp_paths.append(Path(hpp))

    engine_path: Optional[Path] = None
    api_template_path: Optional[Path] = None
    output_path: Path = arguments.output
    api_path: Path = output_path / "api"
    api_path.mkdir(parents=True, exist_ok=True)

    billets: List[common_types.PreprocessorHeaderBillet] = []
    for hpp in headers:
        source_hpp_path = Path(hpp)
        print(f"Processing {hpp}")
        engine_path_candidate = lexgine_utils.extract_engine_path(source_hpp_path)
        if engine_path is None:
            engine_path = engine_path_candidate
            api_template_path = engine_path / "api"
        if engine_path_candidate is None or engine_path_candidate != engine_path:
            raise RuntimeError("Unable to determine engine path")

        if util.is_hpp_path_ignored(source_hpp_path, ignored_hpp_paths):
            continue

        with open(source_hpp_path.as_posix(), 'r') as f:
            header_source_code = f.read()

        lexgine_namespaces: List[common_types.LexgineNamespace] = lexgine_utils.get_lexgine_namespaces(
            header_source_code
        )

        exported_types_set: Set[common_types.LexgineTypeReference]
        header_source_code: str
        exported_types_set, header_source_code = lexgine_utils.list_raw_lexgine_export_references(
            header_source_code,
            lexgine_namespaces
        )
        if len(exported_types_set) > 0:
            billets.append(
                common_types.PreprocessorHeaderBillet(
                    exporting_header_path=source_hpp_path,
                    preprocessor_ready_header_source=header_source_code,
                    exported_types_set=exported_types_set
                )
            )

    common_resource_headers: List[Path] = []
    common_resource_sources: List[Path] = []

    def append_common_resource(resource_path: Path):
        if util.is_source_file(resource_path):
            common_resource_sources.append(resource_path)
        elif util.is_header_file(resource_path):
            common_resource_headers.append(resource_path)

    for e in configuration.get("common_resources", []):
        e = Path(engine_path / e)
        if e.is_file():
            append_common_resource(e.relative_to(engine_path))
        elif e.is_dir():
            files_in_directory = util.fetch_files_from_directory(e)
            for ee in files_in_directory:
                append_common_resource(ee.relative_to(engine_path))

    forced_includes = [Path(e) for e in configuration.get("forced_includes", [])]
    inherited_types_mapping = configuration.get("inherited_types_api_mapping", {})
    engine_runtime_resource_map: Dict[Path, List[common_types.RuntimeApi]] = {}

    for v in inherited_types_mapping.values():
        source_object_header_path = Path(v["source_object_header_path"])
        mapped_header = common_types.RuntimeApi(runtime_api_header_path=Path(v["mapped_object_header_path"]))
        if source_object_header_path not in engine_runtime_resource_map:
            engine_runtime_resource_map[source_object_header_path] = [mapped_header]
        else:
            engine_runtime_resource_map[source_object_header_path].append(mapped_header)

    all_exported_resources: List[common_types.ExportedResourceDescriptor] = []
    inherited_types: List[Tuple[common_types.LexgineNamespace, str]] = []
    using_directives: Dict[str, str] = {}

    api_resources = [Path(e) for e in configuration.get("api_resources", [])]

    shutil.rmtree(api_path, ignore_errors=True)

    for export_billet in billets:
        source_path: Path = export_billet.exporting_header_path
        source_relative_path: Path = lexgine_utils.extract_header_relative_path(source_path, engine_path)
        header_source: str = export_billet.preprocessor_ready_header_source

        try:
            parsed_header = CppHeader(header_source, argType="string")
            using_directives.update(parsed_header.using)
        except CppParseError as e:
            print(f'Problem parsing header "{export_billet.exporting_header_path.as_posix()}": {e}')
            continue

        raw_includes = [Path(inc.strip('"<>')) for inc in parsed_header.includes] + forced_includes
        raw_includes = list(set(raw_includes))

        exported_classes = []
        for e in export_billet.exported_types_set:
            if e.token_type == common_types.TokenType.STRUCT or e.token_type == common_types.TokenType.CLASS:
                exported_classes.append(e)
        exported_namespaces: List[common_types.LexgineNamespace] = lexgine_utils.get_lexgine_namespaces(header_source)

        assert len(source_path.parts) > 1
        api_relative_path = lexgine_utils.create_api_path(source_path)
        dest_path = output_path / api_relative_path
        dest_path.mkdir(parents=True, exist_ok=True)

        if source_relative_path not in engine_runtime_resource_map:
            engine_runtime_resource_map[source_relative_path] = []

        def create_resource_descriptor_for_exported_class(
                parent_resource: Optional[common_types.ExportedResourceDescriptor],
                parsed_class: CppClass, token_type: common_types.TokenType,
                parent_namespace: common_types.LexgineNamespace,
                dependency_names: Iterable[str]
        ) -> common_types.ExportedResourceDescriptor:
            exported_methods_in_class: List[CppMethod] = \
                token_fetch_functions.fetch_list_of_exported_methods_for_class(
                export_billet, parsed_class
            )
            dest_header_name: str = util.convert_camel_case_to_snake_case(parsed_class["name"])
            is_ioc = parser_helpers.is_ioc_class(parsed_class, exported_methods_in_class)
            exported_flags_in_class: List[common_types.LexgineFlags] = \
                token_fetch_functions.fetch_list_of_exported_flags_for_class(
                    header_source, export_billet, parsed_class
            ) if is_ioc else None
            exported_enums_in_class: List[CppEnum] = \
                token_fetch_functions.fetch_list_of_exported_enums_in_class(
                    export_billet, parsed_class
            ) if is_ioc else None
            exported_unions_in_class: List[CppUnion] = \
                token_fetch_functions.fetch_list_of_exported_unions_in_class(
                    export_billet, parsed_class
            ) if is_ioc else None
            rv = common_types.ExportedResourceDescriptor(
                short_name=dest_header_name,
                exporting_header_path=source_relative_path,
                exporting_header_source=header_source,
                runtime_api_header_path=api_relative_path / f"{dest_header_name}.h",
                includes=raw_includes,
                parent_namespace=parent_namespace,
                parsed_class=parsed_class,
                token_type=token_type,
                parsed_methods=exported_methods_in_class,
                exported_flags=exported_flags_in_class,
                exported_enums=exported_enums_in_class,
                exported_unions=exported_unions_in_class,
                nested_resources=None,
                parent_resource=parent_resource,
                is_ioc=is_ioc,
                is_inherited=False,
                dependency_names=dependency_names)

            exported_nested_classes: List[CppClass] = [e for e in parsed_class["nested_classes"]
                                                       if type(e) is CppClass and e["access_in_parent"] == "public"]
            new_inherited_types = [(parent_namespace, e["decl_name"]) for e in parsed_class["inherits"]
                                   if e["access"] == "public"]
            mapped_types_indices = []
            for idx, t in enumerate(new_inherited_types):
                mapped_type = get_inheritance_mapped_type(parent_namespace, t[1], inherited_types_mapping)
                if mapped_type is not None:
                    mapped_types_indices.append(idx)
            for i in mapped_types_indices:
                new_inherited_types.pop(i)
            inherited_types.extend(new_inherited_types)

            if len(exported_nested_classes):
                if parsed_class.get("line_number", -1) <= 0:
                    return rv
                parsed_class_line_number_zb = parsed_class["line_number"] - 1
                parsed_class_begin_idx = util.get_line_offset(header_source, parsed_class_line_number_zb)
                _, parsed_class_end_idx = util.extract_scope(header_source, parsed_class_begin_idx,
                                                             common_types.ScopeType.BRACES)
                parsed_class_end_line_number_zb = util.count_lines_till_index(header_source, 0, parsed_class_end_idx)

                list_of_nested_resources: List[common_types.ExportedResourceDescriptor] = []
                for c in exported_nested_classes:
                    tt = common_types.TokenType.CLASS
                    for ref in exported_classes:
                        if ref.line == c["line_number"] and ref.name == c["name"]:
                            tt = ref.token_type
                            break
                    pn = common_types.LexgineNamespace(
                        f"{parent_namespace.definition}::{parsed_class['name']}",
                        parsed_class_line_number_zb,
                        parsed_class_end_line_number_zb,
                        parsed_class_begin_idx,
                        parsed_class_end_idx
                    )
                    list_of_nested_resources.append(create_resource_descriptor_for_exported_class(rv, c, tt, pn, []))
                rv.nested_resources = list_of_nested_resources

            return rv

        for e in exported_classes:
            parsed_class: Optional[CppClass] = token_fetch_functions.fetch_parsed_class(e, parsed_header.classes)
            if parsed_class is None:  # can only happen if e is a nested class / struct
                continue
            parent_namespace: common_types.LexgineNamespace = lexgine_utils.get_lexgine_namespace_for_location(
                exported_namespaces,
                parsed_class["line_number"] - 1
            )
            desc = create_resource_descriptor_for_exported_class(None, parsed_class, e.token_type, parent_namespace,
                                                                 e.dependency_list)
            engine_runtime_resource_map[source_relative_path].append(desc)
            all_exported_resources.append(desc)

        # Enums, flags, and unions can be nested within classes, so we need to first parse the classes, while removing
        # all the nested types found in them from export_billet.exported_types_set. The remaining types are then
        # processed independently
        exported_enums = []
        exported_unions = []
        exported_flags = []
        for e in export_billet.exported_types_set:
            if e.token_type == common_types.TokenType.ENUM:
                exported_enums.append(e)
            elif e.token_type == common_types.TokenType.UNION:
                exported_unions.append(e)
            elif e.token_type == common_types.TokenType.FLAGS:
                exported_flags.append(e)

        for e in exported_enums + exported_unions:
            parsed_unions = {k: v for k, v in parsed_header.classes.items() if isinstance(v, CppUnion)}
            parsed_enum_or_union: Union[CppEnum, CppUnion] = \
                token_fetch_functions.fetch_parsed_enum_or_union(
                    export_billet,
                    e,
                    parsed_header.enums,
                    parsed_unions
                )
            parent_namespace: common_types.LexgineNamespace = lexgine_utils.get_lexgine_namespace_for_location(
                exported_namespaces, parsed_enum_or_union["line_number"] - 1
            )
            dest_header_name: str = util.convert_camel_case_to_snake_case(parsed_enum_or_union["name"])
            desc = common_types.ExportedResourceDescriptor(
                short_name=dest_header_name,
                exporting_header_path=source_relative_path,
                exporting_header_source=header_source,
                runtime_api_header_path=api_relative_path / f"{dest_header_name}.h",
                includes=raw_includes,
                parent_namespace=parent_namespace,
                parsed_class=parsed_enum_or_union,
                token_type=e.token_type,
                parsed_methods=[],
                exported_flags=None,
                exported_enums=[parsed_enum_or_union] if isinstance(parsed_enum_or_union, CppEnum) else None,
                exported_unions=[parsed_enum_or_union] if isinstance(parsed_enum_or_union, CppUnion) else None,
                nested_resources=None,
                parent_resource=None,
                is_inherited=False,
                is_ioc=False,
                dependency_names=e.dependency_list)
            engine_runtime_resource_map[source_relative_path].append(desc)
            all_exported_resources.append(desc)

        for e in exported_flags:
            parsed_flags: common_types.LexgineFlags = common_types.LexgineFlags(header_source, e.line - 1, e.name)
            parent_namespace: common_types.LexgineNamespace = lexgine_utils.get_lexgine_namespace_for_location(
                exported_namespaces, e.line - 1
            )
            dest_header_name: str = util.convert_camel_case_to_snake_case(parsed_flags.name)
            desc = common_types.ExportedResourceDescriptor(
                short_name=dest_header_name,
                exporting_header_path=source_relative_path,
                exporting_header_source=header_source,
                runtime_api_header_path=api_relative_path / f"{dest_header_name}.h",
                includes=raw_includes,
                parent_namespace=parent_namespace,
                parsed_class=parsed_flags,
                token_type=e.token_type,
                parsed_methods=[],
                exported_flags=[parsed_flags],
                exported_enums=None,
                exported_unions=None,
                nested_resources=None,
                parent_resource=None,
                is_ioc=False,
                is_inherited=False,
                dependency_names=e.dependency_list)
            engine_runtime_resource_map[source_relative_path].append(desc)
            all_exported_resources.append(desc)

    all_ioc_resources: List[common_types.ExportedResourceDescriptor] = []

    all_exported_resources_named_dict = {}
    for r in all_exported_resources:
        parsed_class = r.parsed_class
        if (isinstance(parsed_class, CppClass)
                or isinstance(parsed_class, CppEnum)
                or isinstance(parsed_class, CppUnion)):
            parsed_class_name = f"{r.parent_namespace.definition}::{parsed_class['name']}"
        elif isinstance(parsed_class, common_types.LexgineFlags):
            parsed_class_name = f"{r.parent_namespace.definition}::{parsed_class.name}"
        else:
            raise AssertionError("Invalid exported resource type handling detected")
        all_exported_resources_named_dict[parsed_class_name] = r

    for resource in all_exported_resources:
        patched_includes = []
        for inc in resource.includes:
            if lexgine_utils.is_engine_path(inc):
                if inc in engine_runtime_resource_map:
                    patched_includes += [e.runtime_api_header_path for e in engine_runtime_resource_map[inc]]
                elif inc in common_resource_headers:
                    patched_includes.append(lexgine_utils.convert_engine_path_to_api_path(inc))
            else:
                patched_includes.append(inc)
        for dependency in resource.dependency_names:
            namespace_variants, dependency_type = parser_helpers.get_context_nested_namespace_variations_for_type_desc(
                resource.parent_namespace, dependency
            )
            found_dependency = False
            for variant in namespace_variants:
                full_qualified_dependency_name_candidate = f"{variant}::{dependency_type}"
                if full_qualified_dependency_name_candidate in all_exported_resources_named_dict:
                    patched_includes.append(
                        all_exported_resources_named_dict[full_qualified_dependency_name_candidate]
                        .runtime_api_header_path
                    )
                    found_dependency = True
                    break
            if found_dependency is False:
                parsed_class_name = r.parsed_class.name if isinstance(r.parsed_class, common_types.LexgineFlags) \
                    else r.parsed_class['name']
                raise RuntimeError(f"Dependency {dependency} referred by resource {parsed_class_name} is not found "
                                   f"among exported resources. Have you forgotten to add 'LEXGINE_CPP_API' token to "
                                   f"the resource declaration?")
        resource.includes = patched_includes

        resource_nesting_ravel = resource.ravel()
        for r in resource_nesting_ravel:
            if r.is_ioc is True:
                all_ioc_resources.append(resource)

    for (context_namespace, type_desc) in inherited_types:
        namespace_variants, type_name = parser_helpers.get_context_nested_namespace_variations_for_type_desc(
            context_namespace, type_desc
        )
        for variant in namespace_variants:
            full_qualified_name = f"{variant}::{type_name}"
            if full_qualified_name in all_exported_resources_named_dict:
                all_exported_resources_named_dict[
                    full_qualified_name].is_inherited = True
                break

    common_lib_path: Path = arguments.common_lib
    if not common_lib_path.exists():
        common_lib_path.mkdir(parents=True, exist_ok=True)

    ioc_traits_export: common_types.LexgineDllExportInterface = ic.create_export_interface_imported_opaque_class_traits(
        all_ioc_resources, configuration
    )
    ioc_traits_api: common_types.LexgineRuntimeLinkInterface = ic.create_import_interface_imported_opaque_class_traits(
        all_ioc_resources, configuration
    )
    ioc_traits_export.write(output_path / "engine/core", "_ioc_traits")
    ioc_traits_api.write(common_lib_path, "ioc_traits")

    runtime_api: common_types.LexgineRuntimeLinkInterface = ic.create_import_interface_api_linker(
        all_ioc_resources, configuration
    )
    runtime_api.write(api_path, "runtime")

    build_info: common_types.LexgineRuntimeLinkInterface = ic.create_import_interface_build_info(
        arguments.source_tree / "engine",
        configuration
    )
    build_info.write(output_path / "engine", "build_info")

    for resource in all_exported_resources:
        if type(resource.parsed_class) is CppClass:
            if resource.is_ioc:
                export_interface = ic.create_export_interface_imported_opaque_class(
                    all_ioc_resources, resource, configuration
                )
                import_interface = ic.create_import_interface_imported_opaque_class(
                    all_ioc_resources, resource, inherited_types_mapping, configuration
                )
                export_interface.write(
                    output_path / resource.exporting_header_path.parent,
                    f"_{resource.short_name}"
                )
            else:
                import_interface = ic.create_import_interface_data_class(resource, configuration)
            import_interface.write(output_path / resource.runtime_api_header_path.parent, resource.short_name)

        elif type(resource.parsed_class) is CppEnum or type(resource.parsed_class) is CppUnion:
            import_interface = ic.create_import_interface_enumeration(
                resource, configuration)
            import_interface.write(output_path / resource.runtime_api_header_path.parent, resource.short_name)

        elif type(resource.parsed_class) is common_types.LexgineFlags:
            import_interface = ic.create_import_interface_flags(resource, configuration)
            import_interface.write(output_path / resource.runtime_api_header_path.parent, resource.short_name)
        else:
            raise AssertionError("Exported resource type is not supported")

    # Copy common headers to the runtime
    for hpp in common_resource_headers:
        assert lexgine_utils.is_engine_path(hpp)
        dst = api_path / hpp.relative_to("engine")
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(engine_path / hpp, dst)

    hpp_list: List[str] = []
    cpp_list: List[str] = []
    for hpp in common_resource_headers:
        assert lexgine_utils.is_engine_path(hpp)
        hpp_list.append(f"{(engine_path / hpp).as_posix()}\n")
    with open(common_lib_path / "headers_list.txt", 'w') as f:
        f.writelines(hpp_list)
    for cpp in common_resource_sources:
        assert lexgine_utils.is_engine_path(cpp)
        cpp_list.append(f"{(engine_path / cpp).as_posix()}\n")
    with open(common_lib_path / "sources_list.txt", 'w') as f:
        f.writelines(cpp_list)
    with open(output_path / "engine" / "excluded_sources.txt", 'w') as f:
        f.writelines(cpp_list)


argument_parser = argparse.ArgumentParser(description="Lexgine DLL interface generator")
argument_parser.add_argument("--source-tree", type=Path, help="Path to repository source tree", dest="source_tree")
argument_parser.add_argument("--headers", required=True, help="List of paths to the C++ headers to be parsed",
                             dest="headers")
argument_parser.add_argument("--config", required=True, help="Path to configuration JSON", dest="config")
argument_parser.add_argument("--output", required=True, help="Output directory where to write the generated interfaces",
                             dest="output", type=Path)
argument_parser.add_argument("--common-lib", required=True, help="Path to the engine common statically linked library",
                             dest="common_lib", type=Path)
arguments = argument_parser.parse_args()
main(arguments)

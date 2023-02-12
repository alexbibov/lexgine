import inspect
from typing import Union, Tuple, Optional
from pathlib import Path
from functools import reduce
from CppHeaderParser import CppClass

import global_names
import common_types
import util


def get_template_descriptor(config: dict, interface_kind: common_types.InterfaceKind) -> Union[dict, str]:
    template_entry = inspect.stack()[1][3][len(f"create_{interface_kind.value}_interface_"):]
    return config[f"{interface_kind.value}_interfaces"][template_entry]


def fetch_import_templates(import_templates_desc: dict) -> Tuple[Optional[str], Optional[str]]:
    import_template_cpp_string = None
    import_template_hpp_string = None
    import_template_cpp_path: Optional[Path] = Path(import_templates_desc["source"]) \
        if "source" in import_templates_desc else None
    import_template_hpp_path: Optional[Path] = Path(import_templates_desc["header"]) \
        if "header" in import_templates_desc else None
    if import_template_cpp_path is not None:
        with open(import_template_cpp_path.as_posix(), 'r') as f:
            import_template_cpp_string = global_names.BSD_3CLAUSE_LICENSE + f.read()
    if import_template_hpp_path is not None:
        with open(import_template_hpp_path.as_posix(), 'r') as f:
            import_template_hpp_string = global_names.BSD_3CLAUSE_LICENSE + f.read()
    return import_template_cpp_string, import_template_hpp_string


def create_hpp_guard(namespace: common_types.LexgineNamespace, cpp_class: CppClass) -> str:
    namespace_parts = namespace.definition.split('::')
    hpp_guard = reduce(lambda x, y: f"{x}_{y.upper()}", namespace_parts[1:], namespace_parts[0].upper()) \
        + f"_{util.convert_camel_case_to_snake_case(cpp_class['name']).upper()}_H"
    return hpp_guard
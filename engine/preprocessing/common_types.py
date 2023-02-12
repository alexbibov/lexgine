from __future__ import annotations
import dataclasses
import re
from enum import Enum
from pathlib import Path
from typing import Iterable, Dict, Set, List, Union, Optional
from CppHeaderParser import CppClass, CppEnum, CppMethod, CppUnion
from collections import namedtuple


class LexgineFlags:
    _flag_entry_re = re.compile(
        r"FLAG\s*\(\s*(?P<flag_name>\w+)\s*,\s*(?P<flag_value>\w+)\s*\)")

    def __init__(self, source: str, declaration_begin_offset_idx: int,
                 name: str):
        source = source[declaration_begin_offset_idx:].lstrip(' \n\t\r')
        _flags_declaration_begin_re = re.compile(
            r"BEGIN_FLAGS_DECLARATION\s*\(\s*" + name + r"\s*\)")
        _flag_declaration_end_re = re.compile(
            r"END_FLAGS_DECLARATION\s*\(\s*" + name + r"\s*\)")

        declaration_begin = _flags_declaration_begin_re.search(source)
        declaration_end = _flag_declaration_end_re.search(source)

        if declaration_begin is None or declaration_end is None:
            raise RuntimeError(
                f"Unable to locate declaration of flags '{name}' in provided source string"
            )

        self._declaration_string = source[declaration_begin.start(
        ):declaration_end.end()]
        self._declaration_flags: Dict[str, str] = {}
        self._name = name

        declaration_flag = LexgineFlags._flag_entry_re.search(
            self._declaration_string)
        while declaration_flag is not None:
            self._declaration_flags[
                declaration_flag["flag_name"]] = declaration_flag["flag_value"]
            declaration_flag = LexgineFlags._flag_entry_re.search(
                self._declaration_string, declaration_flag.end())

    @property
    def name(self) -> str:
        return self._name

    @property
    def declaration_string(self) -> str:
        return self._declaration_string

    def __iter__(self):
        return iter(self._declaration_flags.items())


class TokenType(Enum):
    ENUM = "enum"
    CLASS = "class"
    STRUCT = "struct"
    UNION = "union"
    FUNCTION = 4
    FLAGS = 5
    UNKNOWN = 6


class ResourceType(Enum):
    COMMON = 0
    EXPORTED = 1


class FunctionKind(Enum):
    DEFAULT = 0
    CONSTRUCTOR = 1
    DESTRUCTOR = 2


class InterfaceKind(Enum):
    IMPORT = "import"
    EXPORT = "export"


@dataclasses.dataclass(frozen=True)
class LexgineNamespace:
    definition: str
    begin_line_number: int
    end_line_number: int
    begin_index: int
    end_index: int


@dataclasses.dataclass(frozen=True)
class LexgineTypeReference:
    name: str
    line: int
    token_type: TokenType
    parent_namespace: LexgineNamespace
    dependency_list: Iterable[str]


@dataclasses.dataclass(frozen=True)
class PreprocessorHeaderBillet:
    exporting_header_path: Path
    preprocessor_ready_header_source: str
    exported_types_set: Set[LexgineTypeReference]


@dataclasses.dataclass
class RuntimeApi:
    runtime_api_header_path: Path


@dataclasses.dataclass
class ExportedResourceDescriptor(RuntimeApi):
    short_name: str
    exporting_header_path: Path
    exporting_header_source: str
    includes: List[Path]
    parent_namespace: LexgineNamespace
    parsed_class: Union[CppClass, CppEnum, CppUnion, LexgineFlags]
    token_type: TokenType
    parsed_methods: List[CppMethod]
    exported_flags: Optional[List[LexgineFlags]]
    exported_enums: Optional[List[CppEnum]]
    exported_unions: Optional[List[CppUnion]]
    nested_resources: Optional[List[ExportedResourceDescriptor]]
    parent_resource: Optional[ExportedResourceDescriptor]
    dependency_names: Iterable[str]
    is_inherited: bool
    is_ioc: bool

    def ravel(self) -> List[ExportedResourceDescriptor]:
        rv = [self]
        queue: List[ExportedResourceDescriptor] = self.nested_resources.copy() if self.nested_resources is not None \
            else None

        if queue is not None:
            while len(queue) > 0:
                next_element = queue.pop(0)
                if next_element not in rv:
                    rv.append(next_element)
                else:
                    raise AssertionError(
                        "Resource descriptor nesting structure cannot have looping relations"
                    )
                queue += next_element.nested_resources if next_element.nested_resources is not None else []

        return rv


class ScopeType(Enum):
    BRACES = "{}"
    PARENTHESES = "()"
    SQUARE_BRACKETS = "[]"
    ANGULAR_BRACKETS = "<>"


class LexgineDllExportInterface(
        namedtuple("LexgineDllExportInterface", ["source"])):

    def write(self, path: Path, interface_name: str):
        path.mkdir(parents=True, exist_ok=True)
        if self.source is not None:
            with open((path / f"{interface_name}.cpp").as_posix(), 'w') as f:
                f.write(self.source)


class LexgineRuntimeLinkInterface(
        namedtuple("LexgineRuntimeLinkInterface", ["source", "header"])):

    def write(self, path: Path, interface_name: str):
        path.mkdir(parents=True, exist_ok=True)
        if self.header is not None:
            with open((path / f"{interface_name}.h").as_posix(), 'w') as f:
                f.write(self.header)

        if self.source is not None:
            with open((path / f"{interface_name}.cpp").as_posix(), 'w') as f:
                f.write(self.source)


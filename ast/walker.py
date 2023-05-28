from __future__ import annotations

import logging
import os
import re
from enum import Enum
from typing import Optional, List, Tuple
import clang.cindex
from jinja2 import Environment, FileSystemLoader
from jinja2_workarounds import MultiLineInclude

env = Environment(
    loader=FileSystemLoader(os.path.dirname(os.path.realpath(__file__))),
    extensions=[MultiLineInclude],
)

log = logging.getLogger()

INDENT = 4
K = clang.cindex.CursorKind

CommentCommands = List[Tuple[str, str]]


class MemberType(Enum):
    BASIC = 1
    VECTOR = 2
    OPTIONAL = 3
    OPTIONAL_VECTOR = 4


env.globals["MemberType"] = MemberType


def parse_comment_commands(raw_comment: str) -> CommentCommands:
    comment_commands: CommentCommands = []

    def clean_comment_line(l: str) -> str:
        return l.replace("/", "").replace("*", "").strip()

    comment_lines = [
        line for line in map(clean_comment_line, raw_comment.splitlines()) if line != ""
    ]

    for comment in comment_lines:
        parts = comment.split("=", 2)
        if len(parts) != 2:
            continue

        command = parts[0].strip()
        value = parts[1].strip()
        comment_commands.append((command, value))

    return comment_commands


def json_transform(input_str: str, transformation: str) -> str:
    match transformation:
        case "snake_case":
            # TODO: IMPLEMENT
            return re.sub(r"(?<![A-Z])\B[A-Z]", r"_\g<0>", input_str).lower()
        case other:
            log.warn(f"Unknown transformation '{other}', ignoring")
            return input_str


class Member:
    def __init__(
        self,
        name: str,
        member_type: MemberType = MemberType.BASIC,
        type_name: str = "?",
    ) -> None:
        self.name = name
        self.json_name = name
        self.member_type = member_type
        self.type_name = type_name

        self.dont_fail_on_deserialization: bool = False

    def apply_comment_commands(self, comment_commands: CommentCommands) -> None:
        for command, value in comment_commands:
            match command:
                case "json_rename":
                    # Rename the key that this field will use in json terms
                    log.debug(f"Rename json key from {self.json_name} to {value}")
                    self.json_name = value
                case "json_dont_fail_on_deserialization":
                    # Don't fail when an optional object exists and its data is bad
                    log.debug(f"Don't fail on deserialization for {self.name}")
                    self.dont_fail_on_deserialization = bool(value.lower() == "true")
                case "json_transform":
                    # Transform the key from whatever-case to case specified by `value`
                    self.json_name = json_transform(self.json_name, value)
                case "json_inner":
                    # Do nothing on members
                    pass
                case other:
                    log.warn(
                        f"Unknown comment command found: {other} with value {value}"
                    )

    @staticmethod
    def from_field(node: clang.cindex.Cursor) -> Member:
        assert node.type is not None

        name = node.spelling
        member_type = MemberType.BASIC
        type_name = node.type.spelling

        ntargs = node.type.get_num_template_arguments()
        if ntargs > 0:
            overwrite_member_type: Optional[MemberType] = None

            # log.debug(node.type.get_template_argument_type(0).kind)
            # log.debug(node.type.get_template_argument_type(0).spelling)
            # log.debug(node.type.get_template_argument_type(0).get_named_type().spelling)
            # log.debug(node.type.get_template_argument_type(0).get_class_type().spelling)

            for xd in node.get_children():
                match xd.kind:
                    case K.NAMESPACE_REF:
                        # Ignore namespaces
                        pass

                    case K.TEMPLATE_REF:
                        match xd.spelling:
                            case "optional":
                                match overwrite_member_type:
                                    case None:
                                        overwrite_member_type = MemberType.OPTIONAL
                                    case other:
                                        log.warn(
                                            f"Optional cannot be added on top of other member type: {other}"
                                        )

                            case "vector":
                                match overwrite_member_type:
                                    case None:
                                        overwrite_member_type = MemberType.VECTOR
                                    case MemberType.OPTIONAL:
                                        overwrite_member_type = (
                                            MemberType.OPTIONAL_VECTOR
                                        )
                                    case other:
                                        log.warn(
                                            f"Vector cannot be added on top of other member type: {other}"
                                        )

                            case other:
                                log.warn(f"Unhandled template type: {other}")

                    case K.TYPE_REF:
                        type_name = xd.type.spelling

                    case other:
                        log.debug(f"Unhandled child kind type: {other}")

                if overwrite_member_type is not None:
                    member_type = overwrite_member_type

        member = Member(name, member_type, type_name)

        if node.raw_comment is not None:
            comment_commands = parse_comment_commands(node.raw_comment)
            member.apply_comment_commands(comment_commands)

        return member

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, self.__class__):
            return False

        if self.name != other.name:
            return False
        if self.member_type != other.member_type:
            return False
        if self.type_name != other.type_name:
            return False

        return True

    def __repr__(self) -> str:
        match self.member_type:
            case MemberType.BASIC:
                return f"{self.type_name} {self.name}"

            case MemberType.VECTOR:
                return f"std::vector<{self.type_name}> {self.name}"

            case MemberType.OPTIONAL:
                return f"std::optional<{self.type_name}> {self.name}"

            case MemberType.OPTIONAL_VECTOR:
                return f"std::optional<std::vector<{self.type_name}>> {self.name}"


class Struct:
    def __init__(self, name: str) -> None:
        self.name = name
        self.members: List[Member] = []
        self.parent: str = ""
        self.comment_commands: CommentCommands = []
        self.inner_root: str = ""

    @property
    def full_name(self) -> str:
        if self.parent:
            return f"{self.parent}::{self.name}"
        else:
            return self.name

    def __eq__(self, other: object) -> bool:
        if isinstance(other, self.__class__):
            if self.name != other.name:
                return False

            return self.members == other.members
        return False

    def __str__(self) -> str:
        pretty_members = "\n  ".join(map(str, self.members))
        return f"struct {self.name} {{\n  {pretty_members}\n}}"

    def try_value_to_implementation(self) -> str:
        return env.get_template("struct-implementation.tmpl").render(struct=self)

    def try_value_to_definition(self) -> str:
        return env.get_template("struct-definition.tmpl").render(struct=self)

    def apply_comment_commands(self, comment_commands: CommentCommands) -> None:
        for command, value in comment_commands:
            match command:
                case "json_rename":
                    # Do nothing on structs
                    pass
                case "json_dont_fail_on_deserialization":
                    # Do nothing on structs
                    pass
                case "json_transform":
                    # Do nothing on structs
                    pass
                case "json_inner":
                    self.inner_root = value
                case other:
                    log.warn(
                        f"Unknown comment command found: {other} with value {value}"
                    )


class Walker:
    def __init__(self, filename: str) -> None:
        self.filename = filename
        self.real_filepath = os.path.realpath(self.filename)
        self.structs: List[Struct] = []

    def handle_node(self, node: clang.cindex.Cursor, struct: Optional[Struct]) -> bool:
        match node.kind:
            case K.STRUCT_DECL:
                new_struct = Struct(node.spelling)
                if node.raw_comment is not None:
                    new_struct.comment_commands = parse_comment_commands(
                        node.raw_comment
                    )
                    new_struct.apply_comment_commands(new_struct.comment_commands)
                if struct is not None:
                    new_struct.parent = struct.full_name

                for child in node.get_children():
                    self.handle_node(child, new_struct)

                self.structs.append(new_struct)

                return True

            case K.FIELD_DECL:
                type = node.type
                if type is None:
                    # Skip nodes without a type
                    return False

                # log.debug(f"{struct}: {type.spelling} {node.spelling} ({type.kind})")
                if struct:
                    member = Member.from_field(node)
                    member.apply_comment_commands(struct.comment_commands)
                    struct.members.append(member)

            case K.NAMESPACE:
                # Ignore namespaces
                pass

            case K.FUNCTION_DECL:
                # Ignore function declarations
                pass

            case other:
                log.debug(f"unknown kind: {other}")

        return False

    def walk(self, node: clang.cindex.Cursor) -> None:
        node_in_file = bool(
            node.location.file
            and os.path.realpath(node.location.file.name) == self.real_filepath
        )

        handled = False

        if node_in_file:
            handled = self.handle_node(node, None)

        if not handled:
            for child in node.get_children():
                self.walk(child)

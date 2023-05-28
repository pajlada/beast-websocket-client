from __future__ import annotations

from enum import Enum
from typing import Optional, List
import clang.cindex
from jinja2 import Environment, FileSystemLoader
from jinja2_workarounds import MultiLineInclude

env = Environment(loader=FileSystemLoader("."), extensions=[MultiLineInclude])

INDENT = 4
K = clang.cindex.CursorKind


class MemberType(Enum):
    BASIC = 1
    VECTOR = 2
    OPTIONAL = 3
    OPTIONAL_VECTOR = 4


class Member:
    def __init__(
        self,
        name: str,
        json_name: str,
        member_type: MemberType = MemberType.BASIC,
        type_name: str = "?",
    ) -> None:
        self.name = name
        self.json_name = json_name
        self.member_type = member_type
        self.type_name = type_name

    @staticmethod
    def from_field(node: clang.cindex.Cursor) -> Member:
        assert node.type is not None

        name = node.spelling
        json_name = name
        member_type = MemberType.BASIC
        type_name = node.type.spelling

        if node.raw_comment is not None:
            print(node.raw_comment)

            def clean_comment_line(l: str) -> str:
                return l.replace("/", "").replace("*", "").strip()

            comment_lines = [
                line
                for line in map(clean_comment_line, node.raw_comment.splitlines())
                if line != ""
            ]
            print(comment_lines)

            for comment in comment_lines:
                parts = comment.split("=", 2)
                if len(parts) != 2:
                    continue

                command = parts[0].strip()
                value = parts[1].strip()
                print(f"Got command: {command}={value}")

                match command:
                    case "json_rename":
                        # Rename the key that this field will use in json terms
                        json_name = value

        ntargs = node.type.get_num_template_arguments()
        if ntargs > 0:
            overwrite_member_type: Optional[MemberType] = None

            # print(node.type.get_template_argument_type(0).kind)
            # print(node.type.get_template_argument_type(0).spelling)
            # print(node.type.get_template_argument_type(0).get_named_type().spelling)
            # print(node.type.get_template_argument_type(0).get_class_type().spelling)

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
                                        print(
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
                                        print(
                                            f"Vector cannot be added on top of other member type: {other}"
                                        )

                            case other:
                                print(f"Unhandled template type: {other}")

                    case K.TYPE_REF:
                        type_name = xd.type.spelling

                    case other:
                        print(f"Unhandled child kind type: {other}")

                if overwrite_member_type is not None:
                    member_type = overwrite_member_type

        return Member(name, json_name, member_type, type_name)

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
    name: str
    members: List[Member]

    def __init__(self, name: str) -> None:
        self.name = name
        self.members = []

    @staticmethod
    def with_members(name: str, members: List[Member]) -> Struct:
        struct = Struct(name)
        struct.members = members
        return struct

    def __eq__(self, other: object) -> bool:
        if isinstance(other, self.__class__):
            if self.name != other.name:
                return False

            return self.members == other.members
        return False

    def __str__(self) -> str:
        pretty_members = "\n  ".join(map(str, self.members))
        return f"struct {self.name} {{\n  {pretty_members}\n}}"

    def try_value_to(self) -> str:
        env.globals["MemberType"] = MemberType
        template = env.get_template("struct-template.tmpl")
        print(template.render(struct=self))
        return ""


class Walker:
    def __init__(self, filename: str) -> None:
        self.filename = filename
        self.structs: List[Struct] = []

    def handle_node(self, node: clang.cindex.Cursor, struct: Optional[Struct]) -> bool:
        match node.kind:
            case K.STRUCT_DECL:
                # print(f"struct {node.spelling:14}")

                new_struct = Struct(node.spelling)

                for child in node.get_children():
                    self.handle_node(child, new_struct)

                self.structs.append(new_struct)

                return True

            case K.FIELD_DECL:
                type = node.type
                if type is None:
                    # Skip nodes without a type
                    return False

                # print(f"{struct}: {type.spelling} {node.spelling} ({type.kind})")
                if struct:
                    struct.members.append(Member.from_field(node))

            case other:
                print(f"unknown kind: {other}")

        return False

    def walk(self, node: clang.cindex.Cursor) -> None:
        node_in_file = bool(
            node.location.file and node.location.file.name == self.filename
        )

        handled = False

        if node_in_file:
            handled = self.handle_node(node, None)

        if not handled:
            for child in node.get_children():
                self.walk(child)

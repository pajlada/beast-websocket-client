from __future__ import annotations

from typing import Optional, List
import clang.cindex

INDENT = 4
K = clang.cindex.CursorKind


class Member:
    name: str

    @staticmethod
    def with_name(name: str) -> Member:
        member = Member()
        member.name = name
        return member

    def __eq__(self, other: object) -> bool:
        if isinstance(other, self.__class__):
            return self.name == other.name
        return False

    def __repr__(self) -> str:
        return f"var {self.name}"


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

    def __repr__(self) -> str:
        return f"struct {self.name} {{{self.members}}}"


class Walker:
    def __init__(self, filename: str) -> None:
        self.filename = filename
        self.structs: List[Struct] = []

    def handle_node(self, node: clang.cindex.Cursor, struct: Optional[Struct]) -> bool:
        match node.kind:
            case K.STRUCT_DECL:
                print(f"struct {node.spelling:14}")

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

                print(f"{struct}: {type.kind} {node.spelling}")
                if struct:
                    struct.members.append(Member.with_name(node.spelling))
                # canonical = type.get_canonical()
                # if canonical:
                #     print(
                #         f" {struct_context} type.canonical.spelling = {canonical.spelling}"
                #     )
                # ntargs = type.get_num_template_arguments()
                # if ntargs > 0:
                #     print(f"  type.spelling = {type.spelling}")
                #     print(f"  type.get_num_template_arguments = {ntargs}")
                #     # print(f"  type.kind = {node.get_template_argument_kind(0)}")
                #     print(
                #         f"  type.type = {node.get_template_argument_type(0).spelling}"
                #     )
                #     print(f"  type.value = {node.get_template_argument_value(0)}")
                # else:
                #     if type.spelling:
                #         print(f"  type.spelling = {type.spelling}")
                # print()

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

#include <iostream>

#include <clang-c/Index.h>

std::ostream &operator<<(std::ostream &stream, const CXString &str)
{
    stream << clang_getCString(str);
    clang_disposeString(str);
    return stream;
}

int main(int argc, char **argv)
{
    const char *filename = argv[1];

    CXIndex index = clang_createIndex(0, 0);
    const char *const args = "-std=c++17";
    auto *unit = clang_parseTranslationUnit(
        index, filename, &args, 1, nullptr, 0,
        CXTranslationUnit_DetailedPreprocessingRecord |
            CXTranslationUnit_SkipFunctionBodies |
            CXTranslationUnit_IncludeAttributedTypes |
            CXTranslationUnit_VisitImplicitAttributes |
            CXTranslationUnit_ForSerialization | CXTranslationUnit_Incomplete);
    if (unit == nullptr)
    {
        std::cerr << "failed to parse file\n";
        exit(-1);
    }

    auto cursor = clang_getTranslationUnitCursor(unit);

    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData clientData) {
            auto kind = clang_getCursorKind(c);
            if (kind == CXCursor_FieldDecl)
            {
                std::cout << "Found field decl: " << clang_getCursorKind(c)
                          << "\n    " << clang_getCursorSpelling(c) << " - "
                          << clang_getCursorKindSpelling(kind) << "\n";

                auto type = clang_getCursorType(c);
                std::cout << "    " << clang_getTypeSpelling(type) << "\n";
            }

            return CXChildVisit_Recurse;
        },
        nullptr);

    auto asd = clang_getCursorSpelling(cursor);

    std::cout << "asd: " << clang_getCString(asd) << "\n";

    std::cout << "Read file " << filename << "\n";

    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);

    return 0;
}

#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTImporter.h>
#include <clang/AST/ASTNodeTraverser.h>
#include <clang/AST/TextNodeDumper.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Tooling/Tooling.h>

#include <iostream>

using namespace clang;
using namespace tooling;
using namespace ast_matchers;

template <typename Node, typename Matcher>
Node *getFirstDecl(Matcher M, const std::unique_ptr<ASTUnit> &Unit)
{
    auto MB =
        M.bind("bindStr");  // Bind the to-be-matched node to a string key.
    auto MatchRes = match(MB, Unit->getASTContext());
    // We should have at least one match.
    assert(MatchRes.size() >= 1);
    // Get the first matched and bound node.
    Node *Result =
        const_cast<Node *>(MatchRes[0].template getNodeAs<Node>("bindStr"));
    assert(Result);
    return Result;
}

class NodeTreePrinter : public TextTreeStructure
{
    llvm::raw_ostream &OS;

public:
    NodeTreePrinter(llvm::raw_ostream &OS)
        : TextTreeStructure(OS, /* showColors */ false)
        , OS(OS)
    {
    }

    void Visit(const Decl *D)
    {
        OS << D->getDeclKindName() << "Decl";
        if (auto *ND = dyn_cast<NamedDecl>(D))
        {
            OS << " '" << ND->getDeclName() << "'";
        }
    }

    void Visit(const Stmt *S)
    {
        if (!S)
        {
            OS << "<<<NULL>>>";
            return;
        }
        OS << S->getStmtClassName();
        if (auto *E = dyn_cast<DeclRefExpr>(S))
        {
            OS << " '" << E->getDecl()->getDeclName() << "'";
        }
    }

    void Visit(QualType QT)
    {
        OS << "QualType " << QT.split().Quals.getAsString();
    }

    void Visit(const Type *T)
    {
        OS << T->getTypeClassName() << "Type";
    }

    void Visit(const comments::Comment *C, const comments::FullComment *FC)
    {
        OS << C->getCommentKindName();
    }

    void Visit(const CXXCtorInitializer *Init)
    {
        OS << "CXXCtorInitializer";
        if (const auto *F = Init->getAnyMember())
        {
            OS << " '" << F->getNameAsString() << "'";
        }
        else if (auto const *TSI = Init->getTypeSourceInfo())
        {
            OS << " '" << TSI->getType() << "'";
        }
    }

    void Visit(const Attr *A)
    {
        switch (A->getKind())
        {
#define ATTR(X)   \
    case attr::X: \
        OS << #X; \
        break;
#include "clang/Basic/AttrList.inc"
        }
        OS << "Attr";
    }

    void Visit(const OMPClause *C)
    {
        OS << "OMPClause";
    }
    void Visit(const TemplateArgument &A, SourceRange R = {},
               const Decl *From = nullptr, const char *Label = nullptr)
    {
        OS << "TemplateArgument";
        switch (A.getKind())
        {
            case TemplateArgument::Type: {
                OS << " type " << A.getAsType();
                break;
            }
            default:
                break;
        }
    }

    template <typename... T>
    void Visit(T...)
    {
    }
};

class TestASTDumper : public ASTNodeTraverser<TestASTDumper, NodeTreePrinter>
{
    NodeTreePrinter MyNodeRecorder;

public:
    TestASTDumper(llvm::raw_ostream &OS)
        : MyNodeRecorder(OS)
    {
    }
    NodeTreePrinter &doGetNodeDelegate()
    {
        return MyNodeRecorder;
    }
};

template <typename... NodeType>
std::string dumpASTString(NodeType &&...N)
{
    std::string Buffer;
    llvm::raw_string_ostream OS(Buffer);

    TestASTDumper Dumper(OS);

    OS << "\n";

    Dumper.Visit(std::forward<NodeType &&>(N)...);

    return Buffer;
}

template <typename... NodeType>
std::string dumpASTString(TraversalKind TK, NodeType &&...N)
{
    std::string Buffer;
    llvm::raw_string_ostream OS(Buffer);

    TestASTDumper Dumper(OS);
    Dumper.SetTraversalKind(TK);

    OS << "\n";

    Dumper.Visit(std::forward<NodeType &&>(N)...);

    return Buffer;
}

int main()
{
    auto unit = buildASTFromCodeWithArgs(
        R"(
#include <optional>
#include <vector>
//
struct Foo {
    bool fooMember;
};

struct S {
    Foo a;
    std::vector<Foo> as;
    std::optional<Foo> ab;
    int b;
    bool c;
    unsigned d;
};
)",
        {"--std=c++17"}, "test.cc");

    if (!unit)
    {
        std::cerr << "failed to build test.cc ast\n";
        return 1;
    }

    auto &context = unit->getASTContext();

    // context->Node

    auto matcher = cxxRecordDecl().bind("bindStr");

    auto results = match(matcher, context);

    for (const auto &result : results)
    {
        std::cout << "result!!\n";
        auto *node = result.template getNodeAs<CXXRecordDecl>("bindStr");
        if (node == nullptr)
        {
            std::cerr << "couldn't get record node\n";
            continue;
        }
        auto asd = dumpASTString(node);
        std::cout << asd << "\n";
        return 0;
        continue;
        node->getDeclName().dump();

        auto membersMatcher = fieldDecl().bind("fieldStr");

        auto membersResults = match(membersMatcher, node->getASTContext());

        for (const auto &memberResult : membersResults)
        {
            const auto *fieldNode =
                memberResult.template getNodeAs<FieldDecl>("fieldStr");
            if (fieldNode == nullptr)
            {
                std::cerr << "couldn't get field node\n";
                continue;
            }
            std::cout << "  found field: "
                      << fieldNode->getQualifiedNameAsString() << "\n";

            auto type = fieldNode->getType();

            std::cout << "    " << type.getAsString() << "\n";

            type->dump();
            std::cout << "\n";
        }
    }

    // const auto *xd = selectFirst<FieldDecl>();

    return 0;

    std::cerr << "aaaaaaaaa\n";

    std::unique_ptr<ASTUnit> ToUnit = buildASTFromCode("", "to.cc");
    std::unique_ptr<ASTUnit> FromUnit = buildASTFromCode(
        R"(
        class MyClass {
          int m1;
          std::optional<int> m2;
          std::vector<bool> m3;
        };
        )",
        "from.cc");
    auto Matcher = cxxRecordDecl(hasName("MyClass"));
    auto *From = getFirstDecl<CXXRecordDecl>(Matcher, unit);

    ASTImporter Importer(ToUnit->getASTContext(), ToUnit->getFileManager(),
                         FromUnit->getASTContext(), FromUnit->getFileManager(),
                         /*MinimalImport=*/true);
    llvm::Expected<Decl *> ImportedOrErr = Importer.Import(From);
    if (!ImportedOrErr)
    {
        llvm::Error Err = ImportedOrErr.takeError();
        llvm::errs() << "ERROR: " << Err << "\n";
        consumeError(std::move(Err));
        return 1;
    }
    Decl *Imported = *ImportedOrErr;
    Imported->getTranslationUnitDecl()->dump();

    if (llvm::Error Err = Importer.ImportDefinition(From))
    {
        llvm::errs() << "ERROR: " << Err << "\n";
        consumeError(std::move(Err));
        return 1;
    }
    llvm::errs() << "Imported definition.\n";
    Imported->getTranslationUnitDecl()->dump();

    return 0;
};

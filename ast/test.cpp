#include <optional>
#include <vector>

struct Foo {
    struct InnerFoo {
        int asd;
    };

    int test;
};

struct S {
    Foo a;
    std::vector<Foo> as;
    std::optional<Foo> ab;
    int b;
};

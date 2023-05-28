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
    /**
     * 4
     * 5
     * json_rename=vec_of_ints
     **/
    std::vector<int> vecOfInts;
    std::optional<Foo> ab;
    int b;
};

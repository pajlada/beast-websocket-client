#include <vector>

struct Pod {
};

struct Const {
    const int a;
    const bool b;
    const char c;

    std::vector<const bool> d;
    const std::vector<const bool> e;

    const Pod f;
    std::vector<const Pod> g;
};

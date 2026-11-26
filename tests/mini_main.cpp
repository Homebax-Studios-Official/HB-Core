#include "mini_gtest.h"
int main() {
    int failed = 0;
    for (auto& t : hbmini::registry()) {
        hbmini::currentFailed() = false;
        t.fn();
        std::cout << (hbmini::currentFailed() ? "[  FAILED  ] " : "[       OK ] ") << t.suite << "." << t.name << "\n";
        if (hbmini::currentFailed()) ++failed;
    }
    std::cout << hbmini::registry().size() - failed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}

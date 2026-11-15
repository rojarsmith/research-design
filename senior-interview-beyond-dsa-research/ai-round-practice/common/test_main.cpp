// Entry point for the scenario tests. Run with no arguments for every test, or pass
// part of a test name to run only matching tests, for example: tests.exe large
#include "check.hpp"

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    return check::run(argc, argv);
}

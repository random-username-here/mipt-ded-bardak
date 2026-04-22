/**
 * \brief Encoding demo
 *
 * When in repo's build folder:
 *  - run ./pan/libpan-demo to generate demo.add.bmsg by this
 *  - inspect result with ./pan/pan ../pan/examples/add.pan ./demo.add.bmsg
 */
#include "add.hpp"
#include <fstream>

int main() {
    bmsg::CL_test_add msg{123, 2456};

    // lets write it to a file
    std::ofstream f("demo.add.bmsg");
    msg.encode(f, 1, 0);
}

#include <iostream>


#include "libdnf.hpp"


int main() {
    libdnf::Base base;
    base.rpm;
    base.comps;

    std::cout << "OK" << std::endl;
    return 0;
}

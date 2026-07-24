#include "strings.h"
#include <iostream>

int main() {
    strings::string<> str1(64);
    const strings::string str2 = "Morning";
    str1 = str2 + " World";
    str1.print();
    std::cout << "Size: " << str1.size() << '\n';
    std::cout << "Capacity: " << str1.capacity() << '\n';
    return 0;
}
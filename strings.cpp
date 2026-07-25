#include "strings.h"
#include <iostream>

int main() {
    strings::string<> str1(64);
    const strings::string<> str2 = "Morning";
    str1 = str2 + " World";
    str1.print();
    strings::str<> view = "String";
    std::cout << "Size: " << str1.size() << '\n';
    std::cout << "Capacity: " << str1.capacity() << '\n';
    std::cout << "Struct size: " << sizeof(strings::string<>) << '\n';
    std::cout << "Struct size (64 bits): " << sizeof(strings::string<std::uint64_t>) << '\n';
    return 0;
}
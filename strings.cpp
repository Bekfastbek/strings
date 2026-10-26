#include "strings.h"
#include <string>
#include <iostream>

int main() {
    strings::string<> str1(64);
    strings::string<std::uint32_t> strl;
    std::string strings;
    const strings::string<> str2 = "Morning";
    str1 = str2 + " World";
    std::cout << str1 << std::endl;
    const strings::str<> view = "Morning";
    const strings::str<> view1 = str1;
    std::cout << "Size: " << str1.size() << '\n';
    std::cout << "Capacity: " << str1.capacity() << '\n';
    std::cout << "Struct size (32 bits): " << sizeof(strings::string<std::uint32_t>) << '\n';
    std::cout << "Struct size (64 bits): " << sizeof(strings::string<>) << '\n';
    std::cout << "String view: " << view << '\n';
    std::cout << "String view taken from str1: " << view1 << '\n';
    if (view > view1) {
        std::cout << view << " is greater\n";
    } else if (view < view1) {
        std::cout << view1 << " is greater\n";
    } else if (view == view1) {
        std::cout << "Both are equal\n";
    }
    return 0;
}
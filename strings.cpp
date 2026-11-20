#include "strings.h"
#include <print>

int main() {
    strings::string s1 = "This is a test.";
    strings::string view = "test";
    strings::string s2 = s1 + view;
    std::optional<std::uint64_t> index = s1.find(view);
    if (index.has_value()) {
        std::println("{} found at index {}", view, index.value());
    } else {
        std::println("Element not found.");
    }
    return 0;
}
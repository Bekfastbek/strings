#ifndef STRINGS_H
#define STRINGS_H
#include <cstdint>
#include <cstring>
#include <iostream>


// TODO: Need to implement case where doing str64 = str32 + str64 or str32 + str32 is allowed
// TODO: Add a resize function
// TODO: Add a split function
// TODO: Add a reduce operator where it can do str1 - int to reduce the string size of str1
// TODO: Add a += and -= operator overload
// TODO: C interop with functions to export it as a C string
// TODO: Add move and move assign for the rule of 5
// TODO: Documentation and making sure to do that and not be lazy


namespace strings {
    // Immutable string type (I am following the Rust string types which I guess is std::string_view)
    template<typename T = std::uint64_t> // 64 bit since reducing size does nothing in this instance considering str_ is
                                   // 8 byte pointer so size will get padded to 8 byte anyways but free will or whatever
    class str {
        const char *str_;
        T size_;
    public:
        str(const char* s) {
            str_ = s;
            size_ = std::strlen(s);
        }
        str(const char* s, T size) noexcept {
            str_ = s;
            size_ = size;
        }
        constexpr str sub_str(T start, T count) const noexcept {
            return str(str_ + start, count);
        }
        [[nodiscard]] constexpr const char* data() const noexcept {
            return str_;
        }
        constexpr T size() const noexcept {
            return size_;
        }
    };
#if _MSVC_LANG >= 201703L
    str(const char *) -> str<std::uint64_t>;
#elif __cplusplus >= 201703L
    str(const char *) -> str<std::uint64_t>;
#endif

    // Mutable string type
    template<typename T = std::uint32_t> // In this instance the struct size becomes 16 bytes instead of 24 but user can
                              // increase the size to 64 bit if needed but in 99% of cases the higher width isn't needed
    class string {
        static constexpr T SIZE_SSO = sizeof(T) + sizeof(T) + sizeof(char*);
        union {
            struct {
                char *string;
                T capacity; // its size * 2 + 1 unless user presizes during init or does a manual resize
                T size; // size should never reach the last bit so a flag at the last bit should be safe
            } string_;

            struct {
                char string[SIZE_SSO - 1];
                std::uint8_t size; // If the last bit is 1, its SSO else its heap
            } string_small;
        };

        // This basically copies the last byte which lets assume is the 16th byte (so 15 index) and checks its MSB value
        // This part is self reference: 00000000 & 10000000 (0x80) = false 10000000 & 10000000 = true
        [[nodiscard]] bool is_short() const {
            std::uint8_t byte;
            std::memcpy(&byte, reinterpret_cast<const char *>(this) + (SIZE_SSO - 1), 1);
            return byte & 0x80;
        }

        // I will probably thank myself since it reduces a lot of boilerplate
        static void get_len_and_data(const string& object, T& len, const char*& data) {
            if (object.is_short()) {
                data = object.string_small.string;
                len = object.string_small.size & 0x7F;
            } else {
                data = object.string_.string;
                len = object.string_.size;
            }
        }

    public:

        string() : string_small{} {
            string_small.size = 0x80;
        }

        // If user passes an integer in parenthesis, it will force a heap allocation so prevent SSO
        // in case they know that buffer will become heap allocated later on
        explicit string(std::uint64_t bytes) {
            string_.string = new char[bytes]; // I don't think anyone is stupid enough to allocate 0
            string_.string[0] = '\0';
            string_.size = 0;
            string_.capacity = bytes;
        }

        // Assigns string literal to struct
        string(const char *s) {
            T len = std::strlen(s);
            if (len < sizeof(string_small.string)) {
                std::memcpy(string_small.string, s, len + 1);
                string_small.size = len | 0x80;
            } else {
                string_.string = new char[len * 2 + 1];
                std::memcpy(string_.string, s, len + 1);
                string_.size = len;
                string_.capacity = len * 2 + 1;
            }
        }

        // Copy constructor, learned about it today but it copies the struct to the constructor so if i am correct it triggers when strings::string string2 = string1;
        string(const string &other) {
            const char *src;
            T len;
            get_len_and_data(other, len, src);
            if (len < sizeof(string_small.string)) {
                std::memcpy(string_small.string, src, len + 1);
                string_small.size = len | 0x80;
            } else {
                string_.string = new char[len * 2 + 1];
                std::memcpy(string_.string, src, len + 1);
                string_.size = len;
                string_.capacity = len * 2 + 1;
            }
        }

        // Copy assignment constructor, if this is already initialized then it will get overridden i.e. string2 = string1 where string2 already holds a value
        string &operator=(string const &second) {
            if (this == &second) {
                return *this;
            }
            const char *src;
            T len;
            get_len_and_data(second, len, src);
            if (!is_short() && len < string_.capacity) {
                std::memcpy(string_.string, src, len + 1);
                string_.size = len;
                return *this;
            }
            if (!is_short()) {
                delete[] string_.string;
            }
            if (len < sizeof(string_small.string)) {
                std::memcpy(string_small.string, src, len + 1);
                string_small.size = len | 0x80;
            } else {
                string_.string = new char[len * 2 + 1];
                std::memcpy(string_.string, src, len + 1);
                string_.size = len;
                string_.capacity = len * 2 + 1;
            }
            return *this;
        }

        ~string() {
            if (!is_short()) {
                delete[] string_.string;
            }
        }

        T size() const {
            if (is_short()) {
                return string_small.size & 0x7F;
            } else {
                return string_.size;
            }
        }

        // Temporarily added it to just check if an object is SSO or not but I need to flesh it out later
        T capacity() const {
            if (is_short()) {
                return 0;
            } else {
                return string_.capacity;
            }
        }

        // This shouldddd work with both string struct
        friend string operator+(const string &first, const string &second) {
            string res;
            T len1;
            T len2;
            const char *src1;
            const char *src2;
            get_len_and_data(first, len1, src1);
            get_len_and_data(second, len2, src2);
            T total_len = len1 + len2;

            if (total_len < SIZE_SSO - 1) {
                std::memcpy(res.string_small.string, src1, len1);
                std::memcpy(res.string_small.string + len1, src2, len2 + 1);
                res.string_small.size = total_len | 0x80;
            } else {
                res.string_.string = new char[total_len * 2 + 1];
                std::memcpy(res.string_.string, src1, len1);
                std::memcpy(res.string_.string + len1, src2, len2 + 1);
                res.string_.size = total_len;
                res.string_.capacity = total_len * 2 + 1;
            }
            return res;
        }

        // Case where arg1 is struct and arg2 is string literal
        friend string operator+(const string &first, const char *second) {
            string res;
            T len1;
            const char *src1;
            get_len_and_data(first, len1, src1);
            T len2 = strlen(second);
            T total_len = len1 + len2;
            if (total_len < SIZE_SSO - 1) {
                std::memcpy(res.string_small.string, src1, len1);
                std::memcpy(res.string_small.string + len1, second, len2 + 1);
                res.string_small.size = total_len | 0x80;
            } else {
                res.string_.string = new char[total_len * 2 + 1];
                std::memcpy(res.string_.string, src1, len1);
                std::memcpy(res.string_.string + len1, second, len2 + 1);
                res.string_.size = total_len;
                res.string_.capacity = total_len * 2 + 1;
            }
            return res;
        }

        // Case where arg1 is string literal and arg2 is struct
        friend string operator+(const char *second, const string &first) {
            string res;
            T len1;
            const char *src1;
            get_len_and_data(first, len1, src1);
            T len2 = strlen(second);
            T total_len = len1 + len2;
            if (total_len < SIZE_SSO - 1) {
                std::memcpy(res.string_small.string, second, len2);
                std::memcpy(res.string_small.string + len2, src1, len1 + 1);
                res.string_small.size = total_len | 0x80;
            } else {
                res.string_.string = new char[total_len * 2 + 1];
                std::memcpy(res.string_.string, second, len2);
                std::memcpy(res.string_.string + len2, src1, len1 + 1);
                res.string_.size = total_len;
                res.string_.capacity = total_len * 2 + 1;
            }
            return res;
        }

        // temp for debug
        void print() const {
            if (is_short()) {
                std::cout << string_small.string << '\n';
            } else {
                std::cout << string_.string << '\n';
            }
        }
    };
#if _MSVC_LANG >= 201703L
    string(const char *) -> string<std::uint32_t>;
#elif __cplusplus >= 201703L
    string(const char *) -> string<std::uint32_t>;
#endif
}

#endif
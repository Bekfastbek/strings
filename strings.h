#ifndef STRINGS_H
#define STRINGS_H
#include <cstdint>
#include <cstring>
#include <iostream>
#include <ostream>
#include <istream>


// TODO: Need to implement case where doing str64 = str32 + str64 or str32 + str32 is allowed
// TODO: Add a split function
// TODO: Add a reduce operator where it can do str1 - int to reduce the string size of str1
// TODO: Add a +=, -= and == operator overload
// TODO: Custom allocator support where I can just expose a function pointer and do something like alloc(n) instead of new char[n] and the caller can modify what alloc can do
// TODO: Vectorize some parts of the codebase
// TODO: Documentation and making sure to do that and not be lazy


namespace strings {
    // Mutable string type
    template<typename T = std::uint32_t> // In this instance the struct size becomes 16 bytes instead of 24 but user can
    // increase the size to 64 bit if needed but in 99% of cases the higher width isn't needed
    class string {
        static constexpr T SIZE_SSO = sizeof(T) + sizeof(T) + sizeof(char *);
        static constexpr T SIZE_T_MAX = ~(T(1) << (sizeof(T) * 8 - 1));

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
        static void get_len_and_data(const string &object, T &len, const char *&data) {
            if (object.is_short()) {
                data = object.string_small.string;
                len = object.string_small.size & 0x7F;
            } else {
                data = object.string_.string;
                len = object.string_.size;
            }
        }

        static void realloc(string& s1, T capacity) {
            if (s1.is_short()) {
                std::uint8_t len = s1.string_small.size & 0x7F;
                char* temp = new char[capacity + 1];
                std::memcpy(temp, s1.string_small.string, len + 1);
                s1.string_.string = temp;
                s1.string_.size = len;
                s1.string_.capacity = capacity;
            } else {
                char* temp = new char[capacity + 1];
                std::memcpy(temp, s1.string_.string, s1.string_.size + 1);
                delete[] s1.string_.string;
                s1.string_.string = temp;
                s1.string_.capacity = capacity;
            }
        }

        template<typename U>
        friend class str;

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

        // Copy constructor, learned about it today but it copies the struct to the constructor so if i am correct
        // it triggers when strings::string string2 = string1;
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

        // Copy assignment constructor, if this is already initialized then it will get overridden i.e.
        // string2 = string1 where string2 already holds a value
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

        // Move constructor
        string(string &&other) noexcept {
            if (other.is_short()) {
                std::memcpy(string_small.string, other.string_small.string, (other.string_small.size & 0x7F) + 1);
                string_small.size = other.string_small.size;
                other.string_small.string[0] = '\0';
                other.string_small.size = 0x80;
            } else {
                string_.string = other.string_.string;
                string_.size = other.string_.size;
                string_.capacity = other.string_.capacity;
                other.string_.string = nullptr;
                other.string_.size = 0;
                other.string_.capacity = 0;
            }
        }

        // Move assign constructor
        string &operator=(string &&other) noexcept {
            if (this == &other) {
                return *this;
            }
            if (other.is_short()) {
                T len = other.string_small.size & 0x7F;
                const char *src = other.string_small.string;
                // If a user pre-allocates capacity, it will respect that instead of converting back to SSO
                if (!is_short() && string_.capacity > len) {
                    std::memcpy(string_.string, src, len + 1);
                    string_.size = len;
                } else {
                    if (!is_short()) {
                        delete[] string_.string;
                    }
                    std::memcpy(string_small.string, src, len + 1);
                    string_small.size = len | 0x80;
                }
                other.string_small.string[0] = '\0';
                other.string_small.size = 0x80;
            } else {
                if (is_short()) {
                    // So this SHOULD clear SSO state and just move to heap state
                    string_small.string[0] = '\0';
                    string_small.size = 0;
                    string_.string = other.string_.string;
                    string_.size = other.string_.size;
                    string_.capacity = other.string_.capacity;
                    other.string_.string = nullptr;
                    other.string_.size = 0;
                    other.string_.capacity = 0;
                } else {
                    delete[] string_.string;
                    string_.string = other.string_.string;
                    string_.size = other.string_.size;
                    string_.capacity = other.string_.capacity;
                    other.string_.string = nullptr;
                    other.string_.size = 0;
                    other.string_.capacity = 0;
                }
            }
            return *this;
        }

        [[nodiscard]] T size() const {
            if (is_short()) {
                return string_small.size & 0x7F;
            } else {
                return string_.size;
            }
        }

        // Temporarily added it to just check if an object is SSO or not but I need to flesh it out later
        [[nodiscard]] T capacity() const noexcept {
            if (is_short()) {
                return 0;
            } else {
                return string_.capacity;
            }
        }

        [[nodiscard]] const char* data() const noexcept {
            if (is_short()) {
                return string_small.string;
            } else {
                return string_.string;
            }
        }

        void reserve(std::uint64_t bytes) {
            T len;
            const char *src;
            get_len_and_data(*this, len, src);
            if (bytes < len) {
                std::cerr << "strings::string: requested capacity " << bytes << " is smaller than current size " << len
                        << '\n';
                std::abort();
            }
            if (bytes > SIZE_T_MAX) {
                std::cerr << "strings::string: requested capacity " << bytes << " exceeds maximum allowed size (" <<
                        SIZE_T_MAX << ")\n";
                std::abort();
            }
            T current_capacity;
            if (is_short()) {
                current_capacity = sizeof(string_small.string);
            } else {
                current_capacity = string_.capacity;
            }
            if (bytes == current_capacity) {
                return;
            }
            if (bytes < sizeof(string_small.string)) {
                char temp[sizeof(string_small.string)];
                std::memcpy(temp, src, len + 1);
                if (!is_short()) {
                    delete[] string_.string;
                }
                std::memcpy(string_small.string, temp, len + 1);
                string_small.size = len | 0x80;
            } else {
                char *buf = new char[bytes];
                std::memcpy(buf, src, len + 1);
                if (!is_short()) {
                    delete[] string_.string;
                }
                string_.string = buf;
                string_.size = len;
                string_.capacity = bytes;
            }
        }

        friend std::ostream &operator<<(std::ostream &out, string &s) {
            if (s.is_short()) {
                out.write(s.string_small.string, (s.string_small.size & 0x7F));
            } else {
                out.write(s.string_.string, s.string_.size);
            }
            return out;
        }

        friend std::istream &operator>>(std::istream &in, string &s) {
            if (s.is_short()) {
                s.string_small.string[0] = '\0';
                s.string_small.size = 0x80;
            } else {
                // This should be fine since we never read past size so even if there is anything past the null terminator, it won't be read
                s.string_.string[0] = '\0';
                s.string_.size = 0;
            }
            char temp;
            while (in.get(temp) && std::isspace(temp)) {
            }
            if (!in) {
                return in;
            }
            do {
                if (std::isspace(temp)) {
                    in.unget();
                    break;
                }
                if (s.is_short()) {
                    uint8_t sso_len = s.string_small.size & 0x7F;
                    if ((sso_len) == (SIZE_SSO - 1)) {
                        realloc(s, sso_len * 2);
                        s.string_.string[s.string_.size++] = temp;
                    } else {
                        s.string_small.string[sso_len++] = temp;
                        ++s.string_small.size;
                    }
                } else {
                    if ((s.string_.size * 4) >= (s.string_.capacity * 3)) {
                        realloc(s, s.string_.capacity * 2);
                    }
                    s.string_.string[s.string_.size++] = temp;
                }
            } while (in.get(temp));
            if (s.is_short()) {
                s.string_small.string[s.string_small.size & 0x7F] = '\0';
            } else {
                s.string_.string[s.string_.size] = '\0';
            }
            return in;
        }

        void force_dealloc() {
            if (is_short()) {
                string_small.string[0] = '\0';
                string_small.size = 0x80;
            } else {
                delete[] string_.string;
                string_.string = nullptr;
                string_.capacity = 0;
                string_.size = 0x80;
            }
        }
    };
#if _MSVC_LANG >= 201703L
    string(const char *) -> string<std::uint32_t>;
#elif __cplusplus >= 201703L
    string(const char *) -> string<std::uint32_t>;
#endif

    // Immutable string type (functionally same as &str from Rust and std::string_view from libg++)
    template<typename T = std::uint64_t>
    class str {
        const char *str_;
        T size_;

        std::int64_t compare(const str &other) const noexcept {
            T min_len;
            if (size_ < other.size_) {
                min_len = size_;
            } else {
                min_len = other.size_;
            }
            const int cmp = std::memcmp(str_, other.str_, min_len);
            if (cmp != 0) {
                return cmp;
            }
            if (size_ < other.size_) {
                return -1;
            }
            if (size_ > other.size_) {
                return 1;
            }
            return 0;
        }

    public:
        str() {
            str_ = nullptr;
            size_ = 0;
        }

        str(const char *s) {
            str_ = s;
            size_ = std::strlen(s);
        }

        str(const string<> &s) noexcept {
            // Can't use get_len_and_data() because of template datatype mismatch :(
            if (s.is_short()) {
                str_ = s.string_small.string;
                size_ = s.string_small.size & 0x7F;
            } else {
                str_ = s.string_.string;
                size_ = s.string_.size;
            }
        }

        [[nodiscard]] constexpr const char *data() const noexcept {
            return str_;
        }

        constexpr T size() const noexcept {
            return size_;
        }

        friend std::ostream &operator<<(std::ostream &out, const str &s) {
            out.write(s.str_, s.size_);
            return out;
        }

        bool operator==(const str &other) const noexcept {
            return size_ == other.size_ && std::memcmp(str_, other.str_, size_) == 0;
        }

        bool operator!=(const str &other) const noexcept {
            return !(*this == other);
        }

        bool operator<(const str &other) const noexcept {
            return compare(other) < 0;
        }

        bool operator>(const str &other) const noexcept {
            return compare(other) > 0;
        }

        bool operator<=(const str &other) const noexcept {
            return compare(other) <= 0;
        }

        bool operator>=(const str &other) const noexcept {
            return compare(other) >= 0;
        }

        constexpr const char* operator[](T index) const noexcept {
            return *(str_ + index);
        }

        [[nodiscard]] constexpr const char* begin() const noexcept {
            return str_;
        }

        [[nodiscard]] constexpr const char* end() const noexcept {
            return str_ + size_;
        }
    };
#if _MSVC_LANG >= 201703L
    str(const char *) -> str<std::uint64_t>;
#elif __cplusplus >= 201703L
    str(const char *) -> str<std::uint64_t>;
#endif
}

#endif

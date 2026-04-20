#include "frappe/regex.hpp"

#include "frappe/error.hpp"

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

namespace frappe {

class compiled_regex::impl {
  public:
    impl(std::string_view pattern, regex_options opts) : _pattern(pattern) {

        uint32_t options = PCRE2_UTF;

        if (has_option(opts, regex_options::caseless)) {
            options |= PCRE2_CASELESS;
        }
        if (has_option(opts, regex_options::multiline)) {
            options |= PCRE2_MULTILINE;
        }
        if (has_option(opts, regex_options::dotall)) {
            options |= PCRE2_DOTALL;
        }
        if (has_option(opts, regex_options::extended)) {
            options |= PCRE2_EXTENDED;
        }

        int error_code;
        PCRE2_SIZE error_offset;

        _code = pcre2_compile(reinterpret_cast<PCRE2_SPTR>(pattern.data()), pattern.size(), options, &error_code,
                              &error_offset, nullptr);

        if (!_code) {
            PCRE2_UCHAR buffer[256];
            pcre2_get_error_message(error_code, buffer, sizeof(buffer));
            throw regex_error(std::string("PCRE2 compilation failed at offset ") + std::to_string(error_offset) + ": " +
                                  reinterpret_cast<const char *>(buffer),
                              error_code);
        }

        _match_data = pcre2_match_data_create_from_pattern(_code, nullptr);
    }

    ~impl() {
        if (_match_data) {
            pcre2_match_data_free(_match_data);
        }
        if (_code) {
            pcre2_code_free(_code);
        }
    }

    impl(const impl &) = delete;
    impl &operator=(const impl &) = delete;

    impl(impl &&other) noexcept
        : _code(other._code), _match_data(other._match_data), _pattern(std::move(other._pattern)) {
        other._code = nullptr;
        other._match_data = nullptr;
    }

    impl &operator=(impl &&other) noexcept {
        if (this != &other) {
            if (_match_data) pcre2_match_data_free(_match_data);
            if (_code) pcre2_code_free(_code);

            _code = other._code;
            _match_data = other._match_data;
            _pattern = std::move(other._pattern);

            other._code = nullptr;
            other._match_data = nullptr;
        }
        return *this;
    }

    bool match(std::string_view subject) const {
        if (!_code) return false;

        int rc = pcre2_match(_code, reinterpret_cast<PCRE2_SPTR>(subject.data()), subject.size(), 0,
                             PCRE2_ANCHORED | PCRE2_ENDANCHORED, _match_data, nullptr);

        return rc >= 0;
    }

    bool search(std::string_view subject) const {
        if (!_code) return false;

        int rc = pcre2_match(_code, reinterpret_cast<PCRE2_SPTR>(subject.data()), subject.size(), 0, 0, _match_data,
                             nullptr);

        return rc >= 0;
    }

    std::optional<std::string> replace(std::string_view subject, std::string_view replacement) const {
        if (!_code) return std::nullopt;

        PCRE2_SIZE output_size = subject.size() * 2 + replacement.size() + 1;
        std::vector<PCRE2_UCHAR> output(output_size);

        int rc = pcre2_substitute(_code, reinterpret_cast<PCRE2_SPTR>(subject.data()), subject.size(), 0,
                                  PCRE2_SUBSTITUTE_GLOBAL | PCRE2_SUBSTITUTE_OVERFLOW_LENGTH, _match_data, nullptr,
                                  reinterpret_cast<PCRE2_SPTR>(replacement.data()), replacement.size(), output.data(),
                                  &output_size);

        if (rc == PCRE2_ERROR_NOMEMORY) {
            output.resize(output_size + 1);
            rc = pcre2_substitute(_code, reinterpret_cast<PCRE2_SPTR>(subject.data()), subject.size(), 0,
                                  PCRE2_SUBSTITUTE_GLOBAL, _match_data, nullptr,
                                  reinterpret_cast<PCRE2_SPTR>(replacement.data()), replacement.size(), output.data(),
                                  &output_size);
        }

        if (rc < 0) {
            return std::nullopt;
        }

        return std::string(reinterpret_cast<const char *>(output.data()), output_size);
    }

    bool valid() const noexcept { return _code != nullptr; }
    std::string_view pattern() const noexcept { return _pattern; }

  private:
    pcre2_code *_code = nullptr;
    pcre2_match_data *_match_data = nullptr;
    std::string _pattern;
};

compiled_regex::compiled_regex() = default;

compiled_regex::compiled_regex(std::string_view pattern, regex_options opts)
    : _impl(std::make_unique<impl>(pattern, opts)) {}

compiled_regex::~compiled_regex() = default;

compiled_regex::compiled_regex(compiled_regex &&other) noexcept = default;
compiled_regex &compiled_regex::operator=(compiled_regex &&other) noexcept = default;

bool compiled_regex::match(std::string_view subject) const {
    return _impl && _impl->match(subject);
}

bool compiled_regex::search(std::string_view subject) const {
    return _impl && _impl->search(subject);
}

std::optional<std::string> compiled_regex::replace(std::string_view subject, std::string_view replacement) const {
    if (!_impl) return std::nullopt;
    return _impl->replace(subject, replacement);
}

bool compiled_regex::valid() const noexcept {
    return _impl && _impl->valid();
}

std::string_view compiled_regex::pattern() const noexcept {
    return _impl ? _impl->pattern() : std::string_view{};
}

compiled_regex compile_regex(std::string_view pattern, regex_options opts) {
    return compiled_regex(pattern, opts);
}

bool regex_match(const path &p, std::string_view pattern) {
    try {
        compiled_regex re(pattern);
        return re.match(p.string());
    } catch (const regex_error &) {
        return false;
    }
}

bool regex_match(std::string_view name, std::string_view pattern) {
    try {
        compiled_regex re(pattern);
        return re.match(name);
    } catch (const regex_error &) {
        return false;
    }
}

bool regex_match(const path &p, const compiled_regex &re) {
    return re.match(p.string());
}

bool regex_match(std::string_view name, const compiled_regex &re) {
    return re.match(name);
}

bool regex_search(const path &p, std::string_view pattern) {
    try {
        compiled_regex re(pattern);
        return re.search(p.string());
    } catch (const regex_error &) {
        return false;
    }
}

bool regex_search(std::string_view text, std::string_view pattern) {
    try {
        compiled_regex re(pattern);
        return re.search(text);
    } catch (const regex_error &) {
        return false;
    }
}

bool regex_search(const path &p, const compiled_regex &re) {
    return re.search(p.string());
}

bool regex_search(std::string_view text, const compiled_regex &re) {
    return re.search(text);
}

std::optional<path> regex_replace(const path &p, std::string_view pattern, std::string_view replacement) {
    try {
        compiled_regex re(pattern);
        auto result = re.replace(p.string(), replacement);
        if (result) {
            return path(*result);
        }
        return std::nullopt;
    } catch (const regex_error &) {
        return std::nullopt;
    }
}

std::optional<std::string> regex_replace(std::string_view text, std::string_view pattern,
                                         std::string_view replacement) {
    try {
        compiled_regex re(pattern);
        return re.replace(text, replacement);
    } catch (const regex_error &) {
        return std::nullopt;
    }
}

std::optional<path> regex_replace(const path &p, const compiled_regex &re, std::string_view replacement) {
    auto result = re.replace(p.string(), replacement);
    if (result) {
        return path(*result);
    }
    return std::nullopt;
}

std::optional<std::string> regex_replace(std::string_view text, const compiled_regex &re,
                                         std::string_view replacement) {
    return re.replace(text, replacement);
}

} // namespace frappe

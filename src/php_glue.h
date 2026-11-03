#pragma once

// sc-phplib's src/php_glue.h, the parts this extension uses (keep them in step). An area is one
// file: its functions and a startup() that registers its classes; sc_mupdf.cpp lists the areas.

#include <php.h>
#include <zend_exceptions.h>

#include <exception>
#include <optional>
#include <string>
#include <string_view>

namespace sc::php {
    extern zend_class_entry *exception_class;

    // Runs body; a C++ exception becomes an SC\Exception. PHP's own errors don't longjmp out of
    // this (since PHP 8 argument errors are exceptions), so C++ objects in body are destroyed
    // as usual. After it, a function returns when EG(exception) is set (RETURN_THROWS()).
    template<typename Body>
    void guarded(Body body) {
        try {
            body();
        } catch (const std::exception &error) {
            zend_throw_exception(exception_class, error.what(), 0);
        } catch (...) {
            zend_throw_exception(exception_class, "unknown error", 0);
        }
    }

    // guarded(make), then the string it made as the function's return value.
    template<typename Make>
    void return_guarded_string(zval *return_value, Make make) {
        std::string out;
        guarded([&] { out = make(); });
        if (EG(exception)) RETURN_THROWS();
        RETURN_STRINGL(out.data(), out.size());
    }

    inline std::string_view view(const zend_string *text) { return {ZSTR_VAL(text), ZSTR_LEN(text)}; }

    inline zval *find(HashTable *table, const std::string_view key) { return zend_hash_str_find(table, key.data(), key.size()); }

    struct area {
        const zend_function_entry *functions; // ending with ZEND_FE_END; nullptr for none
        void (*startup)();                    // registers its classes; nullptr for none
    };

    area pdf_preview_area();
}

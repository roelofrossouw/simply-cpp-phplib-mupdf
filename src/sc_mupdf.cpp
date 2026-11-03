// The sc_mupdf extension: SC\PdfPreview, and phpinfo()'s line. sc-phplib's AGPL part (MuPDF),
// apart so that sc stays Apache-2.0. Its errors are sc's SC\Exception: loaded after sc (and
// html2pdf, which declares one too) when they're there, and declaring it itself when neither is.

#include "php_glue.h"

#include <ext/standard/info.h>

#include <vector>

namespace sc::php {
    zend_class_entry *exception_class = nullptr;

    namespace {
        // The extension's parts: a new one is a file and a line here.
        const std::vector<area> &areas() {
            static const std::vector<area> all{pdf_preview_area()};
            return all;
        }
    }
}

static PHP_MINIT_FUNCTION(sc_mupdf) {
    sc::php::exception_class = static_cast<zend_class_entry *>(zend_hash_str_find_ptr(CG(class_table), ZEND_STRL("sc\\exception")));
    if (!sc::php::exception_class) {
        zend_class_entry exception;
        INIT_NS_CLASS_ENTRY(exception, "SC", "Exception", nullptr);
        sc::php::exception_class = zend_register_internal_class_ex(&exception, zend_ce_exception);
    }
    for (const auto &each: sc::php::areas())
        if (each.startup) each.startup();
    return SUCCESS;
}

static PHP_MINFO_FUNCTION(sc_mupdf) {
    php_info_print_table_start();
    php_info_print_table_row(2, "simply-cpp MuPDF (sc_mupdf)", "enabled");
    php_info_print_table_row(2, "Version", SC_PHPLIB_MUPDF_VERSION);
    php_info_print_table_row(2, "Provides", "SC\\PdfPreview");
    php_info_print_table_row(2, "License", "AGPL-3.0 (MuPDF)");
    php_info_print_table_end();
}

// Loaded after these when they're there, so that SC\Exception is theirs.
static const zend_module_dep sc_mupdf_deps[] = {
    ZEND_MOD_OPTIONAL("sc")
    ZEND_MOD_OPTIONAL("html2pdf")
    ZEND_MOD_END
};

static zend_module_entry sc_mupdf_module_entry = {
    STANDARD_MODULE_HEADER_EX,
    nullptr,
    sc_mupdf_deps,
    "sc_mupdf",
    nullptr,
    PHP_MINIT(sc_mupdf),
    nullptr,
    nullptr,
    nullptr,
    PHP_MINFO(sc_mupdf),
    SC_PHPLIB_MUPDF_VERSION,
    STANDARD_MODULE_PROPERTIES
};

extern "C" ZEND_DLEXPORT zend_module_entry *get_module() { return &sc_mupdf_module_entry; }

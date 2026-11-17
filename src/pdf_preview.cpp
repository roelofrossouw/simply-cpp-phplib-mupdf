// SC\PdfPreview: sc-mupdf's sc::mupdf::document: a PDF's pages drawn as PNG or JPEG images in
// memory, for previews and thumbnails, with nothing written next to it.
//
//     $pdf = SC\PdfPreview::fromString($bytes);              // or SC\PdfPreview::open($path)
//     $png = $pdf->png(200);                                 // the first page, 200 pixels wide
//     $jpeg = $pdf->jpeg(300, 300, page: 2, quality: 80);    // page 2, within 300 x 300 pixels
//     $pdf->pageCount(); $pdf->pageSize(1);                  // ['width' => 595.0, 'height' => 842.0], points
//
// A size fits the page within width x height pixels, keeping its shape; 0 leaves that side free,
// and both 0 draw it at 72 dpi (a pixel a point). A PDF that can't be read, a wrong password, a
// page that isn't there or a size beyond 10000 pixels is an SC\Exception.

#include "php_glue.h"

#include <sc/mupdf/mupdf.h>

#include <algorithm>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>

namespace {
    zend_class_entry *preview_class = nullptr;
    zend_object_handlers handlers;

    constexpr zend_long largest = 10000; // pixels a side

    struct preview_object {
        sc::mupdf::document *document;
        zend_object std;
    };

    preview_object *object_of(zend_object *object) {
        return reinterpret_cast<preview_object *>(reinterpret_cast<char *>(object) - XtOffsetOf(preview_object, std));
    }

    zend_object *create(zend_class_entry *type) {
        auto *made = static_cast<preview_object *>(zend_object_alloc(sizeof(preview_object), type));
        made->document = nullptr;
        zend_object_std_init(&made->std, type);
        object_properties_init(&made->std, type);
        made->std.handlers = &handlers;
        return &made->std;
    }

    void free_object(zend_object *object) {
        delete object_of(object)->document;
        zend_object_std_dtor(object);
    }

    const sc::mupdf::document &document_of(zval *self) {
        const auto *document = object_of(Z_OBJ_P(self))->document;
        if (!document) throw std::logic_error{"SC\\PdfPreview: not opened (SC\\PdfPreview::open() or SC\\PdfPreview::fromString())"};
        return *document;
    }

    void return_preview(zval *return_value, sc::mupdf::document document) {
        object_init_ex(return_value, preview_class);
        object_of(Z_OBJ_P(return_value))->document = new sc::mupdf::document(std::move(document));
    }

    // Page number (from 1) to sc-mupdf's page.
    sc::mupdf::page page_of(const sc::mupdf::document &document, const zend_long number) {
        if (number < 1 || number > document.page_count())
            throw std::out_of_range{"SC\\PdfPreview: there's no page " + std::to_string(number) + " (it has " + std::to_string(document.page_count()) + ")"};
        return document.page(static_cast<int>(number - 1));
    }

    // Pixels per point that fit the page within width x height (0: that side free; both: 1).
    double scale_for(const sc::mupdf::page &page, const zend_long width, const zend_long height) {
        if (width < 0 || height < 0 || width > largest || height > largest)
            throw std::invalid_argument{"SC\\PdfPreview: width and height must be 0 to " + std::to_string(largest) + " pixels"};
        const auto bounds = page.bounds();
        if (bounds.width() <= 0 || bounds.height() <= 0) throw std::runtime_error{"SC\\PdfPreview: page " + std::to_string(page.number()) + " has no size"};
        double scale = 1;
        if (width && height) scale = std::min(static_cast<double>(width) / bounds.width(), static_cast<double>(height) / bounds.height());
        else if (width) scale = static_cast<double>(width) / bounds.width();
        else if (height) scale = static_cast<double>(height) / bounds.height();
        // A page so long that its free side would pass the limit (a till slip 200 pixels wide).
        if (std::max(bounds.width(), bounds.height()) * scale > static_cast<double>(largest))
            throw std::invalid_argument{"SC\\PdfPreview: page " + std::to_string(page.number()) + " would be over " + std::to_string(largest) + " pixels; give both width and height"};
        return scale;
    }

    // --- opening ---

    ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_open, 0, 1, SC\\PdfPreview, 0)
        ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
        ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, password, IS_STRING, 0, "''")
    ZEND_END_ARG_INFO()

    ZEND_NAMED_FUNCTION(preview_open) {
        zend_string *path = nullptr, *password = nullptr;
        ZEND_PARSE_PARAMETERS_START(1, 2)
            Z_PARAM_PATH_STR(path)
            Z_PARAM_OPTIONAL
            Z_PARAM_STR(password)
        ZEND_PARSE_PARAMETERS_END();
        std::optional<sc::mupdf::document> document;
        sc::php::guarded([&] {
            document.emplace(std::string{sc::php::view(path)}, password ? sc::php::view(password) : std::string_view{});
        });
        if (EG(exception)) RETURN_THROWS();
        return_preview(return_value, std::move(*document));
    }

    ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_from_string, 0, 1, SC\\PdfPreview, 0)
        ZEND_ARG_TYPE_INFO(0, bytes, IS_STRING, 0)
        ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, password, IS_STRING, 0, "''")
    ZEND_END_ARG_INFO()

    ZEND_NAMED_FUNCTION(preview_from_string) {
        zend_string *bytes = nullptr, *password = nullptr;
        ZEND_PARSE_PARAMETERS_START(1, 2)
            Z_PARAM_STR(bytes)
            Z_PARAM_OPTIONAL
            Z_PARAM_STR(password)
        ZEND_PARSE_PARAMETERS_END();
        std::optional<sc::mupdf::document> document;
        sc::php::guarded([&] {
            document.emplace(sc::mupdf::document::from_memory(std::string{sc::php::view(bytes)}, "application/pdf",
                                                              password ? sc::php::view(password) : std::string_view{}));
        });
        if (EG(exception)) RETURN_THROWS();
        return_preview(return_value, std::move(*document));
    }

    // --- the document ---

    ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_page_count, 0, 0, IS_LONG, 0)
    ZEND_END_ARG_INFO()

    ZEND_NAMED_FUNCTION(preview_page_count) {
        ZEND_PARSE_PARAMETERS_NONE();
        zend_long count = 0;
        sc::php::guarded([&] { count = document_of(ZEND_THIS).page_count(); });
        if (EG(exception)) RETURN_THROWS();
        RETURN_LONG(count);
    }

    ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_page_size, 0, 0, IS_ARRAY, 0)
        ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, page, IS_LONG, 0, "1")
    ZEND_END_ARG_INFO()

    // ['width' => ..., 'height' => ...] in points (1/72 inch).
    ZEND_NAMED_FUNCTION(preview_page_size) {
        zend_long number = 1;
        ZEND_PARSE_PARAMETERS_START(0, 1)
            Z_PARAM_OPTIONAL
            Z_PARAM_LONG(number)
        ZEND_PARSE_PARAMETERS_END();
        double width = 0, height = 0;
        sc::php::guarded([&] {
            const auto bounds = page_of(document_of(ZEND_THIS), number).bounds();
            width = bounds.width();
            height = bounds.height();
        });
        if (EG(exception)) RETURN_THROWS();
        array_init_size(return_value, 2);
        add_assoc_double(return_value, "width", width);
        add_assoc_double(return_value, "height", height);
    }

    ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_png, 0, 0, IS_STRING, 0)
        ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, width, IS_LONG, 0, "0")
        ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, height, IS_LONG, 0, "0")
        ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, page, IS_LONG, 0, "1")
    ZEND_END_ARG_INFO()

    // A page as a PNG's bytes.
    ZEND_NAMED_FUNCTION(preview_png) {
        zend_long width = 0, height = 0, number = 1;
        ZEND_PARSE_PARAMETERS_START(0, 3)
            Z_PARAM_OPTIONAL
            Z_PARAM_LONG(width)
            Z_PARAM_LONG(height)
            Z_PARAM_LONG(number)
        ZEND_PARSE_PARAMETERS_END();
        sc::php::return_guarded_string(return_value, [&] {
            const auto page = page_of(document_of(ZEND_THIS), number);
            return page.png(scale_for(page, width, height));
        });
    }

    ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_jpeg, 0, 0, IS_STRING, 0)
        ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, width, IS_LONG, 0, "0")
        ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, height, IS_LONG, 0, "0")
        ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, page, IS_LONG, 0, "1")
        ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, quality, IS_LONG, 0, "85")
    ZEND_END_ARG_INFO()

    // A page as a JPEG's bytes, quality 1 to 100.
    ZEND_NAMED_FUNCTION(preview_jpeg) {
        zend_long width = 0, height = 0, number = 1, quality = 85;
        ZEND_PARSE_PARAMETERS_START(0, 4)
            Z_PARAM_OPTIONAL
            Z_PARAM_LONG(width)
            Z_PARAM_LONG(height)
            Z_PARAM_LONG(number)
            Z_PARAM_LONG(quality)
        ZEND_PARSE_PARAMETERS_END();
        sc::php::return_guarded_string(return_value, [&] {
            if (quality < 1 || quality > 100) throw std::invalid_argument{"SC\\PdfPreview: the quality must be 1 to 100"};
            const auto page = page_of(document_of(ZEND_THIS), number);
            return page.jpeg(scale_for(page, width, height), static_cast<int>(quality));
        });
    }

    const zend_function_entry methods[] = {
        ZEND_NAMED_ME(open, preview_open, arginfo_open, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
        ZEND_NAMED_ME(fromString, preview_from_string, arginfo_from_string, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
        ZEND_NAMED_ME(pageCount, preview_page_count, arginfo_page_count, ZEND_ACC_PUBLIC)
        ZEND_NAMED_ME(pageSize, preview_page_size, arginfo_page_size, ZEND_ACC_PUBLIC)
        ZEND_NAMED_ME(png, preview_png, arginfo_png, ZEND_ACC_PUBLIC)
        ZEND_NAMED_ME(jpeg, preview_jpeg, arginfo_jpeg, ZEND_ACC_PUBLIC)
        ZEND_FE_END
    };

    void startup() {
        zend_class_entry type;
        INIT_NS_CLASS_ENTRY(type, "SC", "PdfPreview", methods);
        preview_class = zend_register_internal_class(&type);
        preview_class->ce_flags |= ZEND_ACC_FINAL | ZEND_ACC_NOT_SERIALIZABLE;
        preview_class->create_object = create;
        std::memcpy(&handlers, zend_get_std_object_handlers(), sizeof(handlers));
        handlers.offset = XtOffsetOf(preview_object, std);
        handlers.free_obj = free_object;
        handlers.clone_obj = nullptr;
    }
}

sc::php::area sc::php::pdf_preview_area() { return {nullptr, startup}; }

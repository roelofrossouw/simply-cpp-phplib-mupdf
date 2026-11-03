<?php
/**
 * Stubs for the sc_mupdf extension (simply-cpp-phplib-mupdf), for IDEs: what it declares, with
 * types and documentation. Not loaded by PHP; the extension itself is sc_mupdf.so.
 *
 * AGPL-3.0, as MuPDF, which draws the pages, is: sc-phplib's own extension (sc) stays Apache-2.0.
 *
 * @link https://github.com/roelofrossouw/simply-cpp-phplib-mupdf
 */

namespace SC;

/** What goes wrong: sc's own, when the sc extension is loaded too; else this extension declares it. */
class Exception extends \Exception
{
}

/**
 * A PDF's pages drawn as PNG or JPEG images in memory, for previews and thumbnails; nothing is
 * written next to it.
 *
 *     $pdf = \SC\PdfPreview::fromString($bytes);              // or \SC\PdfPreview::open($path)
 *     header('Content-Type: image/png');
 *     echo $pdf->png(200);                                    // the first page, 200 pixels wide
 *     $jpeg = $pdf->jpeg(300, 300, page: 2, quality: 80);     // page 2, within 300 x 300 pixels
 *
 * A size fits the page within width x height pixels, keeping its shape: 0 leaves that side free,
 * and both 0 draw it at 72 dpi (a pixel a point). Neither side may pass 10000 pixels. Errors are
 * \SC\Exception (sc's, when the sc extension is loaded too).
 */
final class PdfPreview
{
    /**
     * A PDF file; its password when it's encrypted.
     *
     * @throws Exception When it can't be opened, or the password is wrong.
     */
    public static function open(string $path, string $password = ''): PdfPreview
    {
    }

    /**
     * The same, for a PDF's bytes.
     *
     * @throws Exception When they aren't a PDF it can read, or the password is wrong.
     */
    public static function fromString(string $bytes, string $password = ''): PdfPreview
    {
    }

    public function pageCount(): int
    {
    }

    /**
     * A page's size in points (1/72 inch): ['width' => 595.0, 'height' => 842.0] for A4.
     *
     * @throws Exception For a page it doesn't have (they count from 1).
     */
    public function pageSize(int $page = 1): array
    {
    }

    /**
     * A page as a PNG's bytes.
     *
     * @throws Exception For a page it doesn't have, or a size outside 0 to 10000 pixels.
     */
    public function png(int $width = 0, int $height = 0, int $page = 1): string
    {
    }

    /**
     * A page as a JPEG's bytes, smaller than a PNG for a photo-like page; quality 1 to 100.
     *
     * @throws Exception The same, a quality outside 1 to 100, or on Ubuntu jammy (MuPDF 1.19
     * writes no JPEG).
     */
    public function jpeg(int $width = 0, int $height = 0, int $page = 1, int $quality = 85): string
    {
    }
}

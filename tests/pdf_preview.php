<?php
// SC\PdfPreview: a two-page PDF made here (A4 with a black square, then 200 x 100 points), drawn
// as PNG and JPEG at the sizes asked for; and what it refuses.
require __DIR__ . '/check.inc';

function make_pdf(): string {
    $square = "0 0 0 rg 100 100 200 200 re f";
    $objects = [
        '<< /Type /Catalog /Pages 2 0 R >>',
        '<< /Type /Pages /Kids [3 0 R 5 0 R] /Count 2 >>',
        '<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] /Contents 4 0 R >>',
        '<< /Length ' . strlen($square) . " >>\nstream\n$square\nendstream",
        '<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 100] >>',
    ];
    $pdf = "%PDF-1.4\n";
    $offsets = [];
    foreach ($objects as $i => $object) {
        $offsets[] = strlen($pdf);
        $pdf .= ($i + 1) . " 0 obj\n$object\nendobj\n";
    }
    $xref = strlen($pdf);
    $pdf .= "xref\n0 " . (count($objects) + 1) . "\n0000000000 65535 f \n";
    foreach ($offsets as $offset) $pdf .= sprintf("%010d 00000 n \n", $offset);
    return $pdf . "trailer\n<< /Size " . (count($objects) + 1) . " /Root 1 0 R >>\nstartxref\n$xref\n%%EOF\n";
}

function png_size(string $png): array {
    return array_values(unpack('N2', substr($png, 16, 8)));
}

$pdf = SC\PdfPreview::fromString(make_pdf());
check($pdf->pageCount() === 2, 'two pages');
check($pdf->pageSize() === ['width' => 595.0, 'height' => 842.0], 'the first page, A4 in points');
check($pdf->pageSize(2) === ['width' => 200.0, 'height' => 100.0], 'the second page');

$png = $pdf->png();
check(str_starts_with($png, "\x89PNG\r\n\x1a\n"), 'a PNG');
check(png_size($png) === [595, 842], 'both sides free: a pixel a point');
check(png_size($pdf->png(200)) === [200, 284], 'a width: the height follows (283.03, rounded up as MuPDF does)');
check(png_size($pdf->png(0, 421)) === [298, 421], 'a height: the width follows');
check(png_size($pdf->png(100, 100, 2)) === [100, 50], 'within a box: page 2 is wide');
check(png_size($pdf->png(page: 2, width: 400)) === [400, 200], 'named arguments');
if (function_exists('imagecreatefromstring')) {
    $image = imagecreatefromstring($pdf->png());
    check(imagecolorat($image, 200, 642) === 0 && imagecolorat($image, 50, 50) === 0xffffff, 'the square drawn, black on white');
}

try {
    $jpeg = $pdf->jpeg(200, 0, 1, 70);
    check(str_starts_with($jpeg, "\xff\xd8\xff"), 'a JPEG');
    check(strlen($pdf->jpeg(400, quality: 95)) > strlen($jpeg), 'bigger and better: more bytes');
    if (function_exists('getimagesizefromstring')) check(array_slice(getimagesizefromstring($jpeg), 0, 2) === [200, 284], "the JPEG's size");
} catch (SC\Exception $e) {
    check(str_contains($e->getMessage(), 'MuPDF 1.23'), 'no JPEG only with an old MuPDF (jammy)');
}

check(throws(fn() => $pdf->png(page: 3)) && throws(fn() => $pdf->png(page: 0)) && throws(fn() => $pdf->pageSize(3)), 'a page it does not have');
check(throws(fn() => $pdf->png(-1)) && throws(fn() => $pdf->png(10001)), 'a size outside 0 to 10000');
check(throws(fn() => $pdf->png(0, 10000, 1)) === false, 'the limit itself');
check(throws(fn() => $pdf->jpeg(100, quality: 0)) && throws(fn() => $pdf->jpeg(100, quality: 101)), 'a quality outside 1 to 100');
check(throws(fn() => SC\PdfPreview::fromString('not a PDF')), 'not a PDF');
check(throws(fn() => SC\PdfPreview::open(__DIR__ . '/none.pdf')), 'no file');

$file = tempnam(sys_get_temp_dir(), 'sc-mupdf') . '.pdf';
file_put_contents($file, make_pdf());
check(SC\PdfPreview::open($file)->pageCount() === 2, 'from a file');
unlink($file);

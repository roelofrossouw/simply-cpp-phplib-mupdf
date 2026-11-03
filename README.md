# simply-cpp-phplib-mupdf

The `sc_mupdf` PHP extension: PDF previews for PHP. `SC\PdfPreview` draws a PDF's pages as PNG
or JPEG images in memory, for thumbnails and previews, with nothing written next to it. It's
sc-phplib's AGPL part: [MuPDF](https://mupdf.com), which draws the pages (through
[sc-mupdf](https://github.com/roelofrossouw/simply-cpp-mupdf)), is AGPL-3.0, so this extension is
too, and sc-phplib's `sc` extension stays Apache-2.0.

```php
$pdf = SC\PdfPreview::fromString($bytes);              // or SC\PdfPreview::open($path); a password second
header('Content-Type: image/png');
echo $pdf->png(200);                                    // the first page, 200 pixels wide

$jpeg = $pdf->jpeg(300, 300, page: 2, quality: 80);     // page 2, within 300 x 300 pixels
$pdf->pageCount();                                      // 2
$pdf->pageSize(1);                                      // ['width' => 595.0, 'height' => 842.0], in points
```

| Method | |
|---|---|
| `SC\PdfPreview::open(string $path, string $password = '')` | a PDF file |
| `SC\PdfPreview::fromString(string $bytes, string $password = '')` | a PDF's bytes |
| `pageCount(): int` | its pages |
| `pageSize(int $page = 1): array` | `['width' =>, 'height' =>]` in points (1/72 inch) |
| `png(int $width = 0, int $height = 0, int $page = 1): string` | a page as a PNG |
| `jpeg(int $width = 0, int $height = 0, int $page = 1, int $quality = 85): string` | a page as a JPEG, quality 1 to 100 |

- **Sizes** fit the page within width x height pixels, keeping its shape: 0 leaves that side
  free, both 0 draw it at 72 dpi (a pixel a point). A free side is rounded up, as MuPDF does
  (A4 200 pixels wide is 284 high). Neither side may pass 10000 pixels.
- **Pages** count from 1, as people count.
- **Errors** are `SC\Exception`: a file that can't be read, a wrong password, a page it doesn't
  have, a size or quality out of range. When sc-phplib's `sc` extension is loaded it's sc's
  class (this one loads after it); without it, this extension declares `SC\Exception` itself.
  MuPDF's own messages ("repairing PDF document") aren't printed into the server's log.
- **JPEG** needs MuPDF 1.23 or newer: noble and resolute. On jammy (MuPDF 1.19) `jpeg()`
  throws; `png()` works everywhere.
- An object is for one request; use it from one thread.

**IDEs:** `stubs/sc_mupdf.stub.php`, installed as
`/usr/share/doc/simply-cpp-phplib-mupdf/sc_mupdf.stub.php`, describes it with types and
documentation; a test checks it against the extension.

## Install

```bash
sudo apt install simply-cpp-phplib-mupdf
```

It's enabled for every PHP SAPI (`phpenmod sc_mupdf`, `mods-available/sc_mupdf.ini`, priority
40, after sc's 30) and php-fpm is restarted when it runs. It recommends `simply-cpp-phplib`.

## Building

`scripts/install.sh` builds, tests and installs it: PHP's `php-config` (php-dev) and sc-mupdf
(`simply-cpp-mupdf-dev`, 4.2.1 or newer). On macOS it builds against Homebrew's PHP and isn't
installed into it; `php -d extension=cmake-build-local/sc_mupdf.so` loads it.

## License

GNU Affero General Public License 3.0 (see `LICENSE`), as MuPDF's. A changed copy that people
use over a network must offer them its source (section 13); unchanged, the source is this
repository, sc-mupdf's and mupdf.com's.

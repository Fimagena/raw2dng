# raw2dng
Linux utility for converting raw photo files into DNG, TIFF and JPEG formats.

While this will happily convert most raw formats, it is especially optimised 
for Sony's A7 (ILCE-7) camera. For that camera it produces identical output 
to Adobe's DNG converter (not bit-wise but metadata), including decoding the 
build-in lens correction profiles, etc.

**Compile:** `cmake`, `make`, `make install`

**Dependencies:**
 - libexiv2 (tested with v0.25)
 - libraw (tested with 0.17.1)
 - Adobe's DNG and XMP SDKs (included in source tree - v1.4 / v201412)
 - libexpat
 - libjpeg
 - zlib

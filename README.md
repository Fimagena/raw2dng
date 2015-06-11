# raw2dng
Linux utility for converting raw photo files into DNG format

This is based on Jens Mueller's work (https://github.com/jmue/dngconvert), 
but updated/improved. While it will happily convert most raw formats, it is
especially optimised for Sony's A7 (ILCE-7) camera. For that camera is
produces identical output to Adobe's DNG converter (not bit-wise but metadata),
including decoding the build-in lens correction profiles, etc.

Dependencies:
 - libexiv2 (tested with v0.24)
 - libraw (tested with 0.17-alpha)
 - Adobe's DNG and XMP SDKs (included in source tree - v1.4 / v201412)
 - libexpat
 - libjpeg
 - zlib

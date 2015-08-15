/* Copyright (C) 2015 Fimagena

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public   
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <stdexcept>

#include "rawConverter.h"



void publishProgressUpdate(const char *message) {std::cout << " - " << message << "...\n";}


void raw2dng(std::string rawFilename, std::string dngFilename, std::string dcpFilename, bool embedOriginal) {
    std::cout << "Starting DNG conversion: \"" << rawFilename << "\" to \"" << dngFilename << "\"\n";
    std::time_t startTime = std::time(NULL);

    RawConverter converter;
    converter.openRawFile(rawFilename);
    converter.buildNegative(dcpFilename);
    if (embedOriginal) converter.embedRaw(rawFilename);
    converter.renderImage();
    AutoPtr<dng_preview_list> previews(converter.renderPreviews());
    converter.writeDng(dngFilename, previews.Get());
    // FIXME: check: are we leaking the previews?

    std::cout << "--> Done (" << std::difftime(std::time(NULL), startTime) << " seconds)\n\n";
}


void raw2tiff(std::string rawFilename, std::string tiffFilename, std::string dcpFilename) {
    std::cout << "Starting Tiff conversion: \"" << rawFilename << "\" to \"" << tiffFilename << "\"\n";
    std::time_t startTime = std::time(NULL);

    RawConverter converter;
    converter.openRawFile(rawFilename);
    converter.buildNegative(dcpFilename);
    converter.renderImage();
    AutoPtr<dng_preview_list> previews(converter.renderPreviews());
    converter.writeTiff(tiffFilename, dynamic_cast<const dng_jpeg_preview*>(&previews->Preview(1)));
    // FIXME: check: are we leaking the previews?

    std::cout << "--> Done (" << std::difftime(std::time(NULL), startTime) << " seconds)\n\n";
}


void raw2jpeg(std::string rawFilename, std::string jpegFilename, std::string dcpFilename) {
    std::cout << "Starting JPEG conversion: \"" << rawFilename << "\" to \"" << jpegFilename << "\"\n";
    std::time_t startTime = std::time(NULL);

    RawConverter converter;
    converter.openRawFile(rawFilename);
    converter.buildNegative(dcpFilename);
    converter.renderImage();
    converter.writeJpeg(jpegFilename);

    std::cout << "--> Done (" << std::difftime(std::time(NULL), startTime) << " seconds)\n\n";

}


int main(int argc, const char* argv []) {  
    if (argc == 1) {
        std::cerr << "\n"
                     "raw2dng - DNG converter\n"
                     "Usage: " << argv[0] << " [options] <rawfile>\n"
                     "Valid options:\n"
                     "  -dcp <filename>      use adobe camera profile\n"
                     "  -e                   embed original\n"
                     "  -j                   convert to JPEG instead of DNG\n"
                     "  -t                   convert to TIFF instead of DNG\n"
                     "  -o <filename>        specify output filename\n\n";
        return -1;
    }

    // -----------------------------------------------------------------------------------------
    // Parse command line

    std::string outFilename;
    std::string dcpFilename;
    bool embedOriginal = false, isJpeg = false, isTiff = false;

    int index;
    for (index = 1; index < argc && argv [index][0] == '-'; index++) {
        std::string option = &argv[index][1];
        if (0 == strcmp(option.c_str(), "o"))   outFilename = std::string(argv[++index]);
        if (0 == strcmp(option.c_str(), "dcp")) dcpFilename = std::string(argv[++index]);
        if (0 == strcmp(option.c_str(), "e"))   embedOriginal = true;
        if (0 == strcmp(option.c_str(), "j"))   isJpeg = true;
        if (0 == strcmp(option.c_str(), "t"))   isTiff = true;
    }

    if (index == argc) {
        std::cerr << "No file specified\n";
        return 1;
    }

    std::string rawFilename(argv[index++]);

    // set output filename: if not given in command line, replace raw file extension with .dng
    if (outFilename.empty()) {
        outFilename.assign(rawFilename, 0, rawFilename.find_last_of("."));
        if (isJpeg)      outFilename.append(".jpg");
        else if (isTiff) outFilename.append(".tif");
        else             outFilename.append(".dng");
    }

    // -----------------------------------------------------------------------------------------
    // Call the conversion function

    try {
        if (isJpeg)      raw2jpeg(rawFilename, outFilename, dcpFilename);
        else if (isTiff) raw2tiff(rawFilename, outFilename, dcpFilename);
        else             raw2dng (rawFilename, outFilename, dcpFilename, embedOriginal);
    }
    catch (std::exception& e) {
        std::cerr << "--> Error! (" << e.what() << ")\n\n";
        return -1;
    }

    return 0;
}

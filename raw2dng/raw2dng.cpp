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
#include <memory>

#include "raw2dng.h"
#include "rawConverter.h"

#include <dng_file_stream.h>

dng_file_stream* openFileStream(std::string &outFilename) {
    try {return new dng_file_stream(outFilename.c_str(), true);}
    catch (dng_exception& e) {
        std::stringstream error; error << "Error! (code: " << e.ErrorCode() << ")";
        throw std::runtime_error(error.str());
    }
}


void raw2dng(std::string rawFilename, std::string outFilename, std::string dcpFilename, bool embedOriginal) {
    AutoPtr<dng_file_stream> filestream(openFileStream(outFilename));

    RawConverter converter;
    converter.openRawFile(rawFilename);
    converter.buildNegative(dcpFilename);
    if (embedOriginal) converter.embedRaw(rawFilename);
    converter.renderImage();
    AutoPtr<dng_preview_list> previews(converter.renderPreviews());
    converter.writeDng(*filestream, previews.Get());
    // FIXME: check: are we leaking the previews?
}


void raw2tiff(std::string rawFilename, std::string outFilename, std::string dcpFilename) {
    AutoPtr<dng_file_stream> filestream(openFileStream(outFilename));

    RawConverter converter;
    converter.openRawFile(rawFilename);
    converter.buildNegative(dcpFilename);
    converter.renderImage();
    AutoPtr<dng_preview_list> previews(converter.renderPreviews());
    converter.writeTiff(*filestream, dynamic_cast<const dng_jpeg_preview*>(&previews->Preview(1)));
    // FIXME: check: are we leaking the previews?
}


void raw2jpeg(std::string rawFilename, std::string outFilename, std::string dcpFilename) {
    AutoPtr<dng_file_stream> filestream(openFileStream(outFilename));

    RawConverter converter;
    converter.openRawFile(rawFilename);
    converter.buildNegative(dcpFilename);
    converter.renderImage();
    converter.writeJpeg(*filestream);
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

    // set output filename: if not given in command line, replace raw file extension
    if (outFilename.empty()) {
        outFilename.assign(rawFilename, 0, rawFilename.find_last_of("."));
        if (isJpeg)      outFilename.append(".jpg");
        else if (isTiff) outFilename.append(".tif");
        else             outFilename.append(".dng");
    }

    // -----------------------------------------------------------------------------------------
    // Call the conversion function

    std::cout << "Starting conversion: \"" << rawFilename << "\n";
    std::time_t startTime = std::time(NULL);

    try {
        if (isJpeg)      raw2jpeg(rawFilename, outFilename, dcpFilename);
        else if (isTiff) raw2tiff(rawFilename, outFilename, dcpFilename);
        else             raw2dng (rawFilename, outFilename, dcpFilename, embedOriginal);
    }
    catch (std::exception& e) {
        std::cerr << "--> Error! (" << e.what() << ")\n\n";
        return -1;
    }

    std::cout << "--> Done (" << std::difftime(std::time(NULL), startTime) << " seconds)\n\n";

    return 0;
}

/* This library is free software; you can redistribute it and/or
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
   
   This file uses code from dngconvert from Jens Mueller and others
   (https://github.com/jmue/dngconvert) - Copyright (C) 2011 Jens 
   Mueller (tschensinger at gmx dot de)
*/

#include "config.h"
#include "raw2dng.h"

#include "dng_negative.h"
#include "dng_preview.h"
#include "dng_xmp_sdk.h"
#include "dng_memory_stream.h"
#include "dng_file_stream.h"
#include "dng_render.h"
#include "dng_image_writer.h"
#include "dng_color_space.h"

#include "negativeProcessor.h"
#include "ILCE7processor.h"
#include "dnghost.h"


void publishProgressUpdate(const char *message) {
#ifdef ANDROID
    sendJNIProgressUpdate(message);
#else
    std::cout << " - " << message << "...\n";
#endif
}


void raw2dng(std::string rawFilename, std::string dngFilename, std::string dcpFilename, bool embedOriginal) {
    // -----------------------------------------------------------------------------------------
    // Init SDK and create processor

    std::cout << "Starting DNG conversion:\n";

    dng_xmp_sdk::InitializeSDK();

    AutoPtr<dng_host> host(dynamic_cast<dng_host*>(new DngHost()));
    host->SetSaveDNGVersion(dngVersion_SaveDefault);
    host->SetSaveLinearDNG(false);
    host->SetKeepOriginalFile(true);

    AutoPtr<dng_negative> negative(host->Make_dng_negative());

    publishProgressUpdate("parsing raw file");

    AutoPtr<NegativeProcessor> negProcessor(NegativeProcessor::createProcessor(host, negative, rawFilename.c_str()));

    // -----------------------------------------------------------------------------------------
    // Set DNG-converter specific values we will need later

    dng_string appName; appName.Set("raw2dng");
    dng_string appVersion; appVersion.Set(RAW2DNG_VERSION_STR);
    dng_string appNameVersion(appName); appNameVersion.Append(" "); appNameVersion.Append(RAW2DNG_VERSION_STR);
    dng_date_time_info dateTimeNow; CurrentDateTimeAndZone(dateTimeNow);

    // -----------------------------------------------------------------------------------------
    // Set all metadata and properties

    publishProgressUpdate("processing metadata");

    negProcessor->setDNGPropertiesFromRaw();
    negProcessor->setCameraProfile(dcpFilename.c_str());
    negProcessor->setExifFromRaw(dateTimeNow, appNameVersion);
    negProcessor->setXmpFromRaw(dateTimeNow, appNameVersion);

    // m_negative->SynchronizeMetadata(); // This would write all Exif into XMP - no real reason to do so?
    negative->RebuildIPTC(true);

    // -----------------------------------------------------------------------------------------
    // Backup proprietary data and embed original raw (if requested)

    publishProgressUpdate("backing up vendor data");

    negProcessor->backupProprietaryData();
    if (true == embedOriginal) {
        publishProgressUpdate("embedding raw file");
        negProcessor->embedOriginalRaw(rawFilename.c_str());
    }

    // -----------------------------------------------------------------------------------------
    // Copy raw image data and render image

    publishProgressUpdate("reading raw image data");

    AutoPtr<dng_image> image(negProcessor->buildDNGImage());

    publishProgressUpdate("building preview - linearising");

    negative->SetStage1Image(image);     // Assign Raw image data.
    negative->BuildStage2Image(*host);   // Compute linearized and range mapped image

    publishProgressUpdate("building preview - demosaicing");

    negative->BuildStage3Image(*host);   // Compute demosaiced image (used by preview and thumbnail)

    // -----------------------------------------------------------------------------------------
    // Render JPEG and thumbnail previews

    publishProgressUpdate("building preview - rendering JPEG");

    dng_jpeg_preview *jpeg_preview = new dng_jpeg_preview();
    jpeg_preview->fInfo.fApplicationName.Set_ASCII(appName.Get());
    jpeg_preview->fInfo.fApplicationVersion.Set_ASCII(appVersion.Get());
    jpeg_preview->fInfo.fDateTime = dateTimeNow.Encode_ISO_8601();
    jpeg_preview->fInfo.fColorSpace = previewColorSpace_sRGB;

    dng_render negRender(*host, *negative);
    negRender.SetFinalSpace(dng_space_sRGB::Get());
    negRender.SetFinalPixelType(ttByte);

    dng_image_writer writer;
    negRender.SetMaximumSize(1024);
    AutoPtr<dng_image> negImage(negRender.Render());
    writer.EncodeJPEGPreview(*host, *negImage.Get(), *jpeg_preview, 5);

    AutoPtr<dng_preview> jp(dynamic_cast<dng_preview*>(jpeg_preview));

    publishProgressUpdate("building preview - rendering thumbnail");

    dng_image_preview *thumbnail = new dng_image_preview();
    thumbnail->fInfo.fApplicationName    = jpeg_preview->fInfo.fApplicationName;
    thumbnail->fInfo.fApplicationVersion = jpeg_preview->fInfo.fApplicationVersion;
    thumbnail->fInfo.fDateTime           = jpeg_preview->fInfo.fDateTime;
    thumbnail->fInfo.fColorSpace         = jpeg_preview->fInfo.fColorSpace;

    negRender.SetMaximumSize(256);
    thumbnail->fImage.Reset(negRender.Render());

    AutoPtr<dng_preview> tn(dynamic_cast<dng_preview*>(thumbnail));

    dng_preview_list previewList; previewList.Append(jp); previewList.Append(tn);

    // -----------------------------------------------------------------------------------------
    // Write DNG-image to file

    publishProgressUpdate("writing DNG file");

    dng_file_stream filestream(dngFilename.c_str(), true);
    writer.WriteDNG(*host, filestream, *negative.Get(), &previewList);

    // -----------------------------------------------------------------------------------------

    publishProgressUpdate("cleaning up");

    dng_xmp_sdk::TerminateSDK();

    std::cout << "--> Done\n";
}


int main(int argc, const char* argv []) {  
    if (argc == 1) {
        fprintf(stderr,
                "\n"
                "raw2dng - DNG converter\n"
                "Usage: %s [options] <dngfile>\n"
                "Valid options:\n"
                "  -dcp <filename>      use adobe camera profile\n"
                "  -e                   embed original\n"
                "  -o <filename>        specify output filename\n",
                argv[0]);

        return -1;
    }

    // -----------------------------------------------------------------------------------------
    // Parse command line

    std::string dngFilename;
    std::string dcpFilename;
    bool embedOriginal = false;

    int index;
    for (index = 1; index < argc && argv [index][0] == '-'; index++) {
        std::string option = &argv[index][1];
        if (0 == strcmp(option.c_str(), "o"))   dngFilename = std::string(argv[++index]);
        if (0 == strcmp(option.c_str(), "dcp")) dcpFilename = std::string(argv[++index]);
        if (0 == strcmp(option.c_str(), "e"))   embedOriginal = true;
    }

    if (index == argc) {
        fprintf (stderr, "no file specified\n");
        return 1;
    }

    std::string rawFilename(argv[index++]);

    // set output filename: if not given in command line, replace raw file extension with .dng
    if (dngFilename.empty()) {
        dngFilename.assign(rawFilename, 0, rawFilename.find_last_of("."));
        dngFilename.append(".dng");
    }

    // -----------------------------------------------------------------------------------------
    // Call the conversion function

    try {
        raw2dng(rawFilename, dngFilename, dcpFilename, embedOriginal);
    }
    catch (std::exception& e) {
        return -1;
    }

    return 0;
}

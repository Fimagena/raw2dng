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

#include "config.h"
#include "rawConverter.h"

#include <stdexcept>

#include "dng_negative.h"
#include "dng_preview.h"
#include "dng_xmp_sdk.h"
#include "dng_memory_stream.h"
#include "dng_file_stream.h"
#include "dng_render.h"
#include "dng_image_writer.h"
#include "dng_color_space.h"
#include "dng_exceptions.h"
#include "dng_tag_values.h"
#include "dng_tag_codes.h"
#include "dng_xmp.h"
#include "dng_color_space.h"

#include "negativeProcessor.h"
#include "dnghost.h"


std::function<void(const char*)> RawConverter::m_publishFunction = NULL;


dng_file_stream* openFileStream(const std::string &outFilename) {
    try {return new dng_file_stream(outFilename.c_str(), true);}
    catch (dng_exception& e) {
        std::stringstream error; error << "Error opening output file! (" << e.ErrorCode() << ": " << getDngErrorMessage(e.ErrorCode()) << ")";
        throw std::runtime_error(error.str());
    }
}


RawConverter::RawConverter() {
    // -----------------------------------------------------------------------------------------
    // Init XMP SDK and some global variables we will need

    dng_xmp_sdk::InitializeSDK();

    m_host.Reset(dynamic_cast<dng_host*>(new DngHost()));
    m_host->SetSaveDNGVersion(dngVersion_SaveDefault);
    m_host->SetSaveLinearDNG(false);
    m_host->SetKeepOriginalFile(true);

    m_appName.Set("raw2dng");
    m_appVersion.Set(RAW2DNG_VERSION_STR);
    CurrentDateTimeAndZone(m_dateTimeNow);
}


RawConverter::~RawConverter() {
    dng_xmp_sdk::TerminateSDK();
}


void RawConverter::registerPublisher(std::function<void(const char*)> publisher) {
    m_publishFunction = publisher;
}


void RawConverter::openRawFile(const std::string rawFilename) {
    // -----------------------------------------------------------------------------------------
    // Create processor and parse raw files

    if (m_publishFunction != NULL) m_publishFunction("parsing raw file");

    m_negProcessor.Reset(NegativeProcessor::createProcessor(m_host, rawFilename.c_str()));
}


void RawConverter::buildNegative(const std::string dcpFilename) {
    // -----------------------------------------------------------------------------------------
    // Set all metadata and properties

    if (m_publishFunction != NULL) m_publishFunction("processing metadata");

    m_negProcessor->setDNGPropertiesFromRaw();
    m_negProcessor->setCameraProfile(dcpFilename.c_str());

    dng_string appNameVersion(m_appName); appNameVersion.Append(" "); appNameVersion.Append(m_appVersion.Get());
    m_negProcessor->setExifFromRaw(m_dateTimeNow, appNameVersion);
    m_negProcessor->setXmpFromRaw(m_dateTimeNow, appNameVersion);

    m_negProcessor->getNegative()->RebuildIPTC(true);

    m_negProcessor->backupProprietaryData();

    // -----------------------------------------------------------------------------------------
    // Copy raw sensor data

    if (m_publishFunction != NULL) m_publishFunction("reading raw image data");

    m_negProcessor->buildDNGImage();
}


void RawConverter::embedRaw(const std::string rawFilename) {
    if (m_publishFunction != NULL) m_publishFunction("embedding raw file");
    m_negProcessor->embedOriginalRaw(rawFilename.c_str());
}


void RawConverter::renderImage() {
    // -----------------------------------------------------------------------------------------
    // Render image

    try {
        if (m_publishFunction != NULL) m_publishFunction("building preview - linearising");

        m_negProcessor->getNegative()->BuildStage2Image(*m_host);   // Compute linearized and range-mapped image

        if (m_publishFunction != NULL) m_publishFunction("building preview - demosaicing");

        m_negProcessor->getNegative()->BuildStage3Image(*m_host);   // Compute demosaiced image (used by preview and thumbnail)
    }
    catch (dng_exception& e) {
        std::stringstream error; error << "Error while rendering image from raw! (" << e.ErrorCode() << ": " << getDngErrorMessage(e.ErrorCode()) << ")";
        throw std::runtime_error(error.str());
    }
}


void RawConverter::renderPreviews() {
    // -----------------------------------------------------------------------------------------
    // Render JPEG and thumbnail previews

    m_previewList.Reset(new dng_preview_list());
    dng_render negRender(*m_host, *m_negProcessor->getNegative());

    if (m_publishFunction != NULL) m_publishFunction("building preview - rendering JPEG");

    dng_jpeg_preview *jpeg_preview = new dng_jpeg_preview();
    jpeg_preview->fInfo.fApplicationName.Set_ASCII(m_appName.Get());
    jpeg_preview->fInfo.fApplicationVersion.Set_ASCII(m_appVersion.Get());
    jpeg_preview->fInfo.fDateTime = m_dateTimeNow.Encode_ISO_8601();
    jpeg_preview->fInfo.fColorSpace = previewColorSpace_sRGB;

    negRender.SetMaximumSize(1024);
    AutoPtr<dng_image> negImage(negRender.Render());
    dng_image_writer jpegWriter; jpegWriter.EncodeJPEGPreview(*m_host, *negImage.Get(), *jpeg_preview, 5);
    AutoPtr<dng_preview> jp(dynamic_cast<dng_preview*>(jpeg_preview));
    m_previewList->Append(jp);

    if (m_publishFunction != NULL) m_publishFunction("building preview - rendering thumbnail");

    dng_image_preview *thumbnail = new dng_image_preview();
    thumbnail->fInfo.fApplicationName    = jpeg_preview->fInfo.fApplicationName;
    thumbnail->fInfo.fApplicationVersion = jpeg_preview->fInfo.fApplicationVersion;
    thumbnail->fInfo.fDateTime           = jpeg_preview->fInfo.fDateTime;
    thumbnail->fInfo.fColorSpace         = jpeg_preview->fInfo.fColorSpace;

    negRender.SetMaximumSize(256);
    thumbnail->fImage.Reset(negRender.Render());
    AutoPtr<dng_preview> tn(dynamic_cast<dng_preview*>(thumbnail));
    m_previewList->Append(tn);
}


void RawConverter::writeDng(const std::string outFilename) {
    // -----------------------------------------------------------------------------------------
    // Write DNG-image to file

    if (m_publishFunction != NULL) m_publishFunction("writing DNG file");

    AutoPtr<dng_file_stream> targetFile(openFileStream(outFilename));

    try {
        dng_image_writer dngWriter; dngWriter.WriteDNG(*m_host, *targetFile, *m_negProcessor->getNegative(), m_previewList.Get());
    }
    catch (dng_exception& e) {
        std::stringstream error; error << "Error while writing DNG-file! (" << e.ErrorCode() << ": " << getDngErrorMessage(e.ErrorCode()) << ")";
        throw std::runtime_error(error.str());
    }
}


void RawConverter::writeTiff(const std::string outFilename) {
    // -----------------------------------------------------------------------------------------
    // Render TIFF

    if (m_publishFunction != NULL) m_publishFunction("rendering TIFF");

    dng_render negRender(*m_host, *m_negProcessor->getNegative());
    AutoPtr<dng_image> negImage(negRender.Render());

    // -----------------------------------------------------------------------------------------
    // Write Tiff-image to file

    AutoPtr<dng_file_stream> targetFile(openFileStream(outFilename));

    if (m_publishFunction != NULL) m_publishFunction("writing TIFF file");

    try {
        dng_image_writer tiffWriter; 
        tiffWriter.WriteTIFF(*m_host, *targetFile, *negImage.Get(), piRGB, ccUncompressed,
                             m_negProcessor->getNegative(), &dng_space_sRGB::Get(), NULL,
                             dynamic_cast<const dng_jpeg_preview*>(&m_previewList->Preview(1)));
    }
    catch (dng_exception& e) {
        std::stringstream error; error << "Error while writing TIFF-file! (" << e.ErrorCode() << ": " << getDngErrorMessage(e.ErrorCode()) << ")";
        throw std::runtime_error(error.str());
    }
}


void RawConverter::writeJpeg(const std::string outFilename) {
    // -----------------------------------------------------------------------------------------
    // Render JPEG

    // FIXME: we should render and integrate a thumbnail too

    if (m_publishFunction != NULL) m_publishFunction("rendering JPEG");

    dng_render negRender(*m_host, *m_negProcessor->getNegative());
    AutoPtr<dng_image> negImage(negRender.Render());

    AutoPtr<dng_jpeg_preview> jpeg(new dng_jpeg_preview());
    jpeg->fInfo.fApplicationName.Set_ASCII(m_appName.Get());
    jpeg->fInfo.fApplicationVersion.Set_ASCII(m_appVersion.Get());
    jpeg->fInfo.fDateTime = m_dateTimeNow.Encode_ISO_8601();
    jpeg->fInfo.fColorSpace = previewColorSpace_sRGB;

    dng_image_writer jpegWriter; jpegWriter.EncodeJPEGPreview(*m_host, *negImage.Get(), *jpeg.Get(), 8);

    // -----------------------------------------------------------------------------------------
    // Write JPEG-image to file

    if (m_publishFunction != NULL) m_publishFunction("writing JPEG file");

    AutoPtr<dng_file_stream> targetFile(openFileStream(outFilename));

    const uint8 soiTag[]         = {0xff, 0xd8};
    const uint8 app1Tag[]        = {0xff, 0xe1};
    const char* app1ExifHeader   = "Exif\0";
    const int exifHeaderLength   = 6;
    const uint8 tiffHeader[]     = {0x49, 0x49, 0x2a, 0x00, 0x08, 0x00, 0x00, 0x00};
    const char* app1XmpHeader    = "http://ns.adobe.com/xap/1.0/";
    const int xmpHeaderLength    = 29;
    const char* app1ExtXmpHeader = "http://ns.adobe.com/xmp/extension/";
    const int extXmpHeaderLength = 35;
    const int jfifHeaderLength   = 20;

    // hack: we're overloading the class just to get access to protected members (DNG-SDK doesn't exposure full Put()-function on these)
    class ExifIfds : public exif_tag_set {
    public:
        dng_tiff_directory* getExifIfd() {return &fExifIFD;}
        dng_tiff_directory* getGpsIfd() {return &fGPSIFD;}
        ExifIfds(dng_tiff_directory &directory, const dng_exif &exif, dng_metadata* md) :
            exif_tag_set(directory, exif, md->IsMakerNoteSafe(), md->MakerNoteData(), md->MakerNoteLength(), false) {}
    };

    try {
        // -----------------------------------------------------------------------------------------
        // Build IFD0, ExifIFD, GPSIFD

        dng_metadata* metadata = &m_negProcessor->getNegative()->Metadata();
        metadata->GetXMP()->Set(XMP_NS_DC, "format", "image/jpeg");

        dng_tiff_directory mainIfd;

        tag_uint16 tagOrientation(tcOrientation, metadata->BaseOrientation().GetTIFF());
        mainIfd.Add(&tagOrientation);

        // this is non-standard I believe but let's leave it anyway
        tag_iptc tagIPTC(metadata->IPTCData(), metadata->IPTCLength());
        if (tagIPTC.Count()) mainIfd.Add(&tagIPTC);

        // this creates exif and gps Ifd and also adds the following to mainIfd:
        // datetime, imagedescription, make, model, software, artist, copyright, exifIfd, gpsIfd
        ExifIfds exifSet(mainIfd, *metadata->GetExif(), metadata);
        exifSet.Locate(sizeof(tiffHeader) + mainIfd.Size());

        // we're ignoring YCbCrPositioning, XResolution, YResolution, ResolutionUnit
        // YCbCrCoefficients, ReferenceBlackWhite

        // -----------------------------------------------------------------------------------------
        // Build IFD0, ExifIFD, GPSIFD

        // Write SOI-tag
        targetFile->Put(soiTag, sizeof(soiTag));

        // Write APP1-Exif section: Header...
        targetFile->Put(app1Tag, sizeof(app1Tag));
        targetFile->SetBigEndian(true);
        targetFile->Put_uint16(sizeof(uint16) + exifHeaderLength + sizeof(tiffHeader) + mainIfd.Size() + exifSet.Size());
        targetFile->Put(app1ExifHeader, exifHeaderLength);

        // ...and TIFF structure
        targetFile->SetLittleEndian(true);
        targetFile->Put(tiffHeader, sizeof(tiffHeader));
        mainIfd.Put(*targetFile, dng_tiff_directory::offsetsRelativeToExplicitBase, sizeof(tiffHeader));
        exifSet.getExifIfd()->Put(*targetFile, dng_tiff_directory::offsetsRelativeToExplicitBase, sizeof(tiffHeader) + mainIfd.Size());
        exifSet.getGpsIfd()->Put(*targetFile, dng_tiff_directory::offsetsRelativeToExplicitBase, sizeof(tiffHeader) + mainIfd.Size() + exifSet.getExifIfd()->Size());

        // Write APP1-XMP if required
        if (metadata->GetXMP()) {
            AutoPtr<dng_memory_block> stdBlock, extBlock;
            dng_string extDigest;
            metadata->GetXMP()->PackageForJPEG(stdBlock, extBlock, extDigest);

            targetFile->Put(app1Tag, sizeof(app1Tag));
            targetFile->SetBigEndian(true);
            targetFile->Put_uint16(sizeof(uint16) + xmpHeaderLength + stdBlock->LogicalSize());
            targetFile->Put(app1XmpHeader, xmpHeaderLength);
            targetFile->Put(stdBlock->Buffer(), stdBlock->LogicalSize());

            if (extBlock.Get()) {
                // we only support one extended block, if XMP is >128k the file will probably be corrupted
                targetFile->Put(app1Tag, sizeof(app1Tag));
                targetFile->SetBigEndian(true);
                targetFile->Put_uint16(sizeof(uint16) + extXmpHeaderLength + extDigest.Length() + sizeof(uint32) + sizeof(uint32) + extBlock->LogicalSize());
                targetFile->Put(app1ExtXmpHeader, extXmpHeaderLength);
                targetFile->Put(extDigest.Get(), extDigest.Length());
                targetFile->Put_uint32(extBlock->LogicalSize());
                targetFile->Put_uint32(stdBlock->LogicalSize());
                targetFile->Put(extBlock->Buffer(), extBlock->LogicalSize());
            }
        }

        // write remaining JPEG structure/data from libjpeg minus the JFIF-header
        targetFile->Put((uint8*) jpeg->fCompressedData->Buffer() + jfifHeaderLength, jpeg->fCompressedData->LogicalSize() - jfifHeaderLength);

        targetFile->Flush();
    }
    catch (dng_exception& e) {
        std::stringstream error; error << "Error while writing JPEG-file! (" << e.ErrorCode() << ": " << getDngErrorMessage(e.ErrorCode()) << ")";
        throw std::runtime_error(error.str());
    }
}

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

#include "DNGprocessor.h"

#include <stdexcept>

#include <dng_camera_profile.h>
#include <dng_file_stream.h>
#include <dng_xmp.h>
#include <dng_info.h>

#include <exiv2/image.hpp>


DNGprocessor::DNGprocessor(AutoPtr<dng_host> &host, LibRaw *rawProcessor, Exiv2::Image::AutoPtr &rawImage)
                             : NegativeProcessor(host, rawProcessor, rawImage) {
    // -----------------------------------------------------------------------------------------
    // Re-read source DNG using DNG SDK - we're ignoring the LibRaw/Exiv2 data structures from now on

    std::string file(m_RawImage->io().path());

    try {
        dng_file_stream stream(file.c_str());

        dng_info info;
        info.Parse(*(m_host.Get()), stream);
        info.PostParse(*(m_host.Get()));
        if (!info.IsValidDNG()) throw dng_exception(dng_error_bad_format);

        m_negative->Parse(*(m_host.Get()), stream, info);
        m_negative->PostParse(*(m_host.Get()), stream, info);
        m_negative->ReadStage1Image(*(m_host.Get()), stream, info);
        m_negative->ReadTransparencyMask(*(m_host.Get()), stream, info);
        m_negative->ValidateRawImageDigest(*(m_host.Get()));
    }
    catch (const dng_exception &except) {throw except;}
    catch (...) {throw dng_exception(dng_error_unknown);}
}


void DNGprocessor::setDNGPropertiesFromRaw() {
    // -----------------------------------------------------------------------------------------
    // Raw filename

    std::string file(m_RawImage->io().path());
    size_t found = std::min(file.rfind("\\"), file.rfind("/"));
    if (found != std::string::npos) file = file.substr(found + 1, file.length() - found - 1);
    m_negative->SetOriginalRawFileName(file.c_str());
}


void DNGprocessor::setCameraProfile(const char *dcpFilename) {
    AutoPtr<dng_camera_profile> prof(new dng_camera_profile);

    if (strlen(dcpFilename) > 0) {
        dng_file_stream profStream(dcpFilename);
        if (!prof->ParseExtended(profStream))
            throw std::runtime_error("Could not parse supplied camera profile file!");
        m_negative->AddProfile(prof);
    }
    else {
        // -----------------------------------------------------------------------------------------
        // Don't do anything, since we're using whatever's already in the DNG
    }
}


void DNGprocessor::setExifFromRaw(const dng_date_time_info &dateTimeNow, const dng_string &appNameVersion) {
    // -----------------------------------------------------------------------------------------
    // We use whatever's in the source DNG and just update date and software

    dng_exif *negExif = m_negative->GetExif();
    negExif->fDateTime = dateTimeNow;
    negExif->fSoftware = appNameVersion;
}


void DNGprocessor::setXmpFromRaw(const dng_date_time_info &dateTimeNow, const dng_string &appNameVersion) {
    // -----------------------------------------------------------------------------------------
    // We use whatever's in the source DNG and just update some base tags

    dng_xmp *negXmp = m_negative->GetXMP();
    negXmp->UpdateDateTime(dateTimeNow);
    negXmp->UpdateMetadataDate(dateTimeNow);
    negXmp->SetString(XMP_NS_XAP, "CreatorTool", appNameVersion);
    negXmp->Set(XMP_NS_DC, "format", "image/dng");
    negXmp->SetString(XMP_NS_PHOTOSHOP, "DateCreated", m_negative->GetExif()->fDateTimeOriginal.Encode_ISO_8601());
}


void DNGprocessor::backupProprietaryData() {
    // -----------------------------------------------------------------------------------------
    // No-op, we use whatever's in the source DNG
}


void DNGprocessor::buildDNGImage() {
    // -----------------------------------------------------------------------------------------
    // No-op, since we've already read the stage 1 image
}

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

   This file uses code from dngconvert from Jens Mueller and others
   (https://github.com/jmue/dngconvert) - Copyright (C) 2011 Jens 
   Mueller (tschensinger at gmx dot de)
*/

#include <dng_rational.h>
#include <dng_string.h>

#include <libraw/libraw.h>
#include <exiv2/exif.hpp>

#include "variousVendorProcessor.h"


VariousVendorProcessor::VariousVendorProcessor(AutoPtr<dng_host> &host, LibRaw *rawProcessor, Exiv2::Image::UniquePtr &rawImage)
                                             : NegativeProcessor(host, rawProcessor, rawImage) {}


void setString(uint32 inInt, dng_string *outString) {
    char tmp_char[256];
    snprintf(tmp_char, sizeof(tmp_char), "%i", inInt);
    outString->Set_ASCII(tmp_char);
}


void VariousVendorProcessor::setDNGPropertiesFromRaw() {
    NegativeProcessor::setDNGPropertiesFromRaw();

    // -----------------------------------------------------------------------------------------
    // Vendor specific crop size data

    uint32 cropWidth = 1 >> 30, cropHeight = 1 >> 30, leftMargin = 1 >> 30, topMargin = 1 >> 30;

    // Nikon -----------------------------------------------------------------------------------
    uint32 nikonCrop[16];
    if (getRawExifTag("Exif.Nikon3.CropHiSpeed", nikonCrop, 7) == 7) {
        // TODO: this seems to be actually the activearea?
        cropWidth = nikonCrop[3];  
        cropHeight = nikonCrop[4];
        leftMargin = nikonCrop[5]; 
        topMargin = nikonCrop[6];
    }
    if (getRawExifTag("Exif.Nikon3.CaptureOutput", nikonCrop, 16) == 16) {
        // this tag seems to be hardly used, could probably delete in favour of the CropHiSpeed one...
        cropWidth  = (nikonCrop[15] >> 24) + (nikonCrop[14] >> 16) + (nikonCrop[13] >> 8) + nikonCrop[12];
        cropHeight = (nikonCrop[11] >> 24) + (nikonCrop[10] >> 16) + (nikonCrop[ 9] >> 8) + nikonCrop[ 8];
    }

    // Sony ------------------------------------------------------------------------------------
    getRawExifTag("Exif.Sony2.FullImageSize", 0, &cropHeight);
    getRawExifTag("Exif.Sony2.FullImageSize", 1, &cropWidth);

    // Olympus ---------------------------------------------------------------------------------
    getRawExifTag("Exif.Olympus.ImageWidth", 0, &cropWidth);
    getRawExifTag("Exif.Olympus.ImageHeight", 0, &cropHeight);

    getRawExifTag("Exif.OlympusIp.CropLeft", 0, &leftMargin);
    getRawExifTag("Exif.OlympusIp.CropTop", 0, &topMargin);
    getRawExifTag("Exif.OlympusIp.CropWidth", 0, &cropWidth);
    getRawExifTag("Exif.OlympusIp.CropHeight", 0, &cropHeight);

    getRawExifTag("Exif.OlympusRi.CropLeft", 0, &leftMargin);
    getRawExifTag("Exif.OlympusRi.CropTop", 0, &topMargin);
    getRawExifTag("Exif.OlympusRi.CropWidth", 0, &cropWidth);
    getRawExifTag("Exif.OlympusRi.CropHeight", 0, &cropHeight);

    // Pentax ----------------------------------------------------------------------------------
    getRawExifTag("Exif.Pentax.RawImageSize", 0, &cropWidth);
    getRawExifTag("Exif.Pentax.RawImageSize", 1, &cropHeight);

    // Panasonic -------------------------------------------------------------------------------
    getRawExifTag("Exif.Panasonic.ImageWidth", 0, &cropWidth);
    getRawExifTag("Exif.Panasonic.ImageHeight", 0, &cropHeight);

    // Set crop --------------------------------------------------------------------------------
    // this looks complicated but just checks that we're in bounds before setting crop 
    if ((cropWidth != 1 >> 30) && (cropHeight != 1>> 30)) {
        if ((leftMargin == 1 >>30) && (cropWidth <= m_RawProcessor->imgdata.sizes.width))
            leftMargin = (m_RawProcessor->imgdata.sizes.width - cropWidth) / 2;
        if ((topMargin == 1 >> 30) && (cropHeight <= m_RawProcessor->imgdata.sizes.height))
            topMargin = (m_RawProcessor->imgdata.sizes.height - cropHeight) / 2;
        if (((leftMargin + cropWidth) <= m_RawProcessor->imgdata.sizes.width) &&
            ((topMargin + cropHeight) <= m_RawProcessor->imgdata.sizes.height)) {
            m_negative->SetDefaultCropOrigin(leftMargin, topMargin);
            m_negative->SetDefaultCropSize(cropWidth, cropHeight);
        }
    }
}


void VariousVendorProcessor::setExifFromRaw(const dng_date_time_info &dateTimeNow, const dng_string &appNameVersion) {
    NegativeProcessor::setExifFromRaw(dateTimeNow, appNameVersion);

    dng_exif *negExif = m_negative->GetExif();

    // -----------------------------------------------------------------------------------------
    // Read various proprietary MakerNote information and adjust Exif. This is not complete
    // (Exiv2 can decode more than this) but better than nothing

    uint32 tmp_uint32 = 0;
    dng_urational tmp_urat(0, 0);

    // -----------------------------------------------------------------------------------------
    // Nikon Makernotes

    getRawExifTag("Exif.Nikon3.Lens", negExif->fLensInfo, 4);
    if (getRawExifTag("Exif.NikonLd1.LensIDNumber", 0, &tmp_uint32)) setString(tmp_uint32, &negExif->fLensID);
    if (getRawExifTag("Exif.NikonLd2.LensIDNumber", 0, &tmp_uint32)) setString(tmp_uint32, &negExif->fLensID);
    if (getRawExifTag("Exif.NikonLd3.LensIDNumber", 0, &tmp_uint32)) setString(tmp_uint32, &negExif->fLensID);
    if (getRawExifTag("Exif.NikonLd2.FocusDistance", 0, &tmp_uint32)) negExif->fSubjectDistance = dng_urational(static_cast<uint32>(pow(10.0, tmp_uint32/40.0)), 100);
    if (getRawExifTag("Exif.NikonLd3.FocusDistance", 0, &tmp_uint32)) negExif->fSubjectDistance = dng_urational(static_cast<uint32>(pow(10.0, tmp_uint32/40.0)), 100);
    getRawExifTag("Exif.NikonLd1.LensIDNumber", &negExif->fLensName);
    getRawExifTag("Exif.NikonLd2.LensIDNumber", &negExif->fLensName);
    getRawExifTag("Exif.NikonLd3.LensIDNumber", &negExif->fLensName);
    getRawExifTag("Exif.Nikon3.ShutterCount", 0, &negExif->fImageNumber);
    if (getRawExifTag("Exif.Nikon3.SerialNO", &negExif->fCameraSerialNumber)) {
        negExif->fCameraSerialNumber.Replace("NO=", "", false);
        negExif->fCameraSerialNumber.TrimLeadingBlanks();
        negExif->fCameraSerialNumber.TrimTrailingBlanks();
    }
    getRawExifTag("Exif.Nikon3.SerialNumber", &negExif->fCameraSerialNumber);

    // checked
    if (negExif->fISOSpeedRatings[0] == 0) {
        getRawExifTag("Exif.Nikon1.ISOSpeed", 1, &negExif->fISOSpeedRatings[0]);
        getRawExifTag("Exif.Nikon2.ISOSpeed", 1, &negExif->fISOSpeedRatings[0]);
        getRawExifTag("Exif.Nikon3.ISOSpeed", 1, &negExif->fISOSpeedRatings[0]);
    }

    // -----------------------------------------------------------------------------------------
    // Canon Makernotes

    if (getRawExifTag("Exif.CanonCs.MaxAperture", 0, &tmp_uint32)) negExif->fMaxApertureValue = dng_urational(tmp_uint32, 32);
    if (getRawExifTag("Exif.CanonSi.SubjectDistance", 0, &tmp_uint32) && (tmp_uint32 > 0)) negExif->fSubjectDistance = dng_urational(tmp_uint32, 100);
    if (getRawExifTag("Exif.Canon.SerialNumber", 0, &tmp_uint32)) {
        setString(tmp_uint32, &negExif->fCameraSerialNumber);
        negExif->fCameraSerialNumber.TrimLeadingBlanks();
        negExif->fCameraSerialNumber.TrimTrailingBlanks();
    }
    if (getRawExifTag("Exif.CanonCs.ExposureProgram", 0, &tmp_uint32)) {
        switch (tmp_uint32) {
            case 1: negExif->fExposureProgram = 2; break;
            case 2: negExif->fExposureProgram = 4; break;
            case 3: negExif->fExposureProgram = 3; break;
            case 4: negExif->fExposureProgram = 1; break;
            default: break;
        }
    }
    if (getRawExifTag("Exif.CanonCs.MeteringMode", 0, &tmp_uint32)) {
        switch (tmp_uint32) {
            case 1: negExif->fMeteringMode = 3; break;
            case 2: negExif->fMeteringMode = 1; break;
            case 3: negExif->fMeteringMode = 5; break;
            case 4: negExif->fMeteringMode = 6; break;
            case 5: negExif->fMeteringMode = 2; break;
            default: break;
        }
    }

    uint32 canonLensUnits = 1, canonLensType = 65535;
    if (getRawExifTag("Exif.CanonCs.Lens", 2, &tmp_urat)) canonLensUnits = tmp_urat.n;
    if (getRawExifTag("Exif.CanonCs.Lens", 0, &tmp_urat)) negExif->fLensInfo[1] = dng_urational(tmp_urat.n, canonLensUnits);
    if (getRawExifTag("Exif.CanonCs.Lens", 1, &tmp_urat)) negExif->fLensInfo[0] = dng_urational(tmp_urat.n, canonLensUnits);
    if (getRawExifTag("Exif.Canon.FocalLength", 1, &tmp_urat)) negExif->fFocalLength = dng_urational(tmp_urat.n, canonLensUnits);
    if (getRawExifTag("Exif.CanonCs.LensType", 0, &canonLensType) && (canonLensType != 65535)) setString(canonLensType, &negExif->fLensID);
    if (!getRawExifTag("Exif.Canon.LensModel", &negExif->fLensName) && (canonLensType != 65535)) 
        getRawExifTag("Exif.CanonCs.LensType", &negExif->fLensName);

    getRawExifTag("Exif.Canon.OwnerName", &negExif->fOwnerName);
    getRawExifTag("Exif.Canon.OwnerName", &negExif->fArtist);

    if (getRawExifTag("Exif.Canon.FirmwareVersion", &negExif->fFirmware)) {
        negExif->fFirmware.Replace("Firmware", "");
        negExif->fFirmware.Replace("Version", "");
        negExif->fFirmware.TrimLeadingBlanks();
        negExif->fFirmware.TrimTrailingBlanks();
    }

    getRawExifTag("Exif.Canon.FileNumber", 0, &negExif->fImageNumber);
    getRawExifTag("Exif.CanonFi.FileNumber", 0, &negExif->fImageNumber);

    // checked
    if (negExif->fISOSpeedRatings[0] == 0) 
        getInterpretedRawExifTag("Exif.CanonSi.ISOSpeed", 0, &negExif->fISOSpeedRatings[0]);

    // -----------------------------------------------------------------------------------------
    // Pentax Makernotes

    uint32 pentaxLensId1, pentaxLensId2;
    getRawExifTag("Exif.Pentax.LensType", &negExif->fLensName);
    if ((getRawExifTag("Exif.Pentax.LensType", 0, &pentaxLensId1)) && (getRawExifTag("Exif.Pentax.LensType", 1, &pentaxLensId2))) {
        char tmp_char[256];
        snprintf(tmp_char, sizeof(tmp_char), "%i %i", pentaxLensId1, pentaxLensId2);
        negExif->fLensID.Set_ASCII(tmp_char);
    }

    // checked
    if (negExif->fISOSpeedRatings[0] == 0) 
        getInterpretedRawExifTag("Exif.Pentax.ISO", 0, &negExif->fISOSpeedRatings[0]);

    // -----------------------------------------------------------------------------------------
    // Olympus Makernotes

    getRawExifTag("Exif.OlympusEq.SerialNumber", &negExif->fCameraSerialNumber);
    getRawExifTag("Exif.OlympusEq.LensSerialNumber", &negExif->fLensSerialNumber);
    getRawExifTag("Exif.OlympusEq.LensModel", &negExif->fLensName);
    if (getRawExifTag("Exif.OlympusEq.MinFocalLength", 0, &tmp_uint32)) negExif->fLensInfo[0] = dng_urational(tmp_uint32, 1);
    if (getRawExifTag("Exif.OlympusEq.MaxFocalLength", 0, &tmp_uint32)) negExif->fLensInfo[1] = dng_urational(tmp_uint32, 1);

    // -----------------------------------------------------------------------------------------
    // Panasonic Makernotes

    getRawExifTag("Exif.Panasonic.LensType", &negExif->fLensName);
    getRawExifTag("Exif.Panasonic.LensSerialNumber", &negExif->fLensSerialNumber);

    // checked
    if (negExif->fISOSpeedRatings[0] == 0) 
        if (getRawExifTag("Exif.Panasonic.ProgramISO", 0, &tmp_uint32) && (tmp_uint32 != 65535))
            negExif->fISOSpeedRatings[0] = tmp_uint32;

    // -----------------------------------------------------------------------------------------
    // Samsung Makernotes

    // checked
    getRawExifTag("Exif.Samsung2.FirmwareName", &negExif->fFirmware);
    getRawExifTag("Exif.Samsung2.LensType", &negExif->fLensID);
    if ((negExif->fFocalLengthIn35mmFilm == 0) && getRawExifTag("Exif.Samsung2.FocalLengthIn35mmFormat", 0, &tmp_uint32))
        negExif->fFocalLengthIn35mmFilm = tmp_uint32 / 10;

    // -----------------------------------------------------------------------------------------
    // Sony Makernotes

    if (getRawExifTag("Exif.Sony2.LensID", 0, &tmp_uint32)) setString(tmp_uint32, &negExif->fLensID);
}


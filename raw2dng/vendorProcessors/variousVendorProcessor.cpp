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

static uint32 pentaxIsoCodes[65][2] = {
       {3, 50},      {4, 64},      {5, 80},     {6, 100},     {7, 125},    {8, 160},    {9, 200},    {10, 250}, 
     {11, 320},    {12, 400},    {13, 500},    {14, 640},    {15, 800},  {16, 1000},  {17, 1250},   {18, 1600}, 
    {19, 2000},   {20, 2500},   {21, 3200},   {22, 4000},   {23, 5000},  {24, 6400},  {25, 8000},  {26, 10000}, 
   {27, 12800},  {28, 16000},  {29, 20000},  {30, 25600},  {31, 32000}, {32, 40000}, {33, 51200},  {34, 64000}, 
   {35, 80000}, {36, 102400}, {37, 128000}, {38, 160000}, {39, 204800},    {50, 50},  {100, 100},   {200, 200}, 
     {258, 50},    {259, 70},   {260, 100},   {261, 140},   {262, 200},  {263, 280},  {264, 400},   {265, 560}, 
    {266, 800},  {267, 1100},  {268, 1600},  {269, 2200},  {270, 3200}, {271, 4500}, {272, 6400},  {273, 9000}, 
  {274, 12800}, {275, 18000}, {276, 25600}, {277, 36000}, {278, 51200},  {400, 400},  {800, 800}, {1600, 1600}, 
  {3200, 3200}};


VariousVendorProcessor::VariousVendorProcessor(AutoPtr<dng_host> &host, AutoPtr<dng_negative> &negative, 
                                               LibRaw *rawProcessor, Exiv2::Image::AutoPtr &rawImage)
                                             : NegativeProcessor(host, negative, rawProcessor, rawImage) {}


void setString(uint32 inInt, dng_string *outString) {
    char tmp_char[256];
    snprintf(tmp_char, sizeof(tmp_char), "%i", inInt);
    outString->Set_ASCII(tmp_char);
}


void VariousVendorProcessor::setDNGPropertiesFromRaw() {
    NegativeProcessor::setDNGPropertiesFromRaw();

    // -----------------------------------------------------------------------------------------
    // Sony default crop origin/size

    uint32 imageSizeHV[2];
    if (getRawExifTag("Exif.Sony2.FullImageSize", imageSizeHV, 2) == 2) {
        int frameH = (imageSizeHV[1] > m_RawProcessor->imgdata.sizes.width ) ? 0 : m_RawProcessor->imgdata.sizes.width  - imageSizeHV[1];
        int frameV = (imageSizeHV[0] > m_RawProcessor->imgdata.sizes.height) ? 0 : m_RawProcessor->imgdata.sizes.height - imageSizeHV[0];

        m_negative->SetDefaultCropOrigin(frameH / 2, frameV / 2);
        m_negative->SetDefaultCropSize(imageSizeHV[0], imageSizeHV[1]);
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
        if (getRawExifTag("Exif.Nikon1.ISOSpeed", 1, &tmp_uint32)) negExif->fISOSpeedRatings[0] = tmp_uint32;
        if (getRawExifTag("Exif.Nikon2.ISOSpeed", 1, &tmp_uint32)) negExif->fISOSpeedRatings[0] = tmp_uint32;
        if (getRawExifTag("Exif.Nikon3.ISOSpeed", 1, &tmp_uint32)) negExif->fISOSpeedRatings[0] = tmp_uint32;
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
    if ((negExif->fISOSpeedRatings[0] == 0) && getRawExifTag("Exif.CanonSi.ISOSpeed", 0, &tmp_uint32)) 
        negExif->fISOSpeedRatings[0] = tmp_uint32;

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
    if ((negExif->fISOSpeedRatings[0] == 0) && getRawExifTag("Exif.Pentax.ISO", 0, &tmp_uint32)) {
        for (int i = 0; i < 65; i++)
            if (pentaxIsoCodes[i][0] == tmp_uint32) negExif->fISOSpeedRatings[0] = pentaxIsoCodes[i][1];
    }

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


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

#include "negativeProcessor.h"
#include "vendorProcessors/DNGprocessor.h"
#include "vendorProcessors/ILCE7processor.h"
#include "vendorProcessors/FujiProcessor.h"
#include "vendorProcessors/variousVendorProcessor.h"

#include <stdexcept>

#include <dng_simple_image.h>
#include <dng_camera_profile.h>
#include <dng_file_stream.h>
#include <dng_memory_stream.h>
#include <dng_xmp.h>

#include <zlib.h>

#include <exiv2/image.hpp>
#include <exiv2/xmp_exiv2.hpp>
#include <libraw/libraw.h>

const char* getDngErrorMessage(int errorCode) {
    switch (errorCode) {
        default:
        case 100000: return "Unknown error";
        case 100003: return "Processing stopped by user (or host application) request";
        case 100004: return "Necessary host functionality is not present";
        case 100005: return "Out of memory";
        case 100006: return "File format is not valid";
        case 100007: return "Matrix has wrong shape, is badly conditioned, or similar problem";
        case 100008: return "Could not open file";
        case 100009: return "Error reading file";
        case 100010: return "Error writing file";
        case 100011: return "Unexpected end of file";
        case 100012: return "File is damaged in some way";
        case 100013: return "Image is too big to save as DNG";
        case 100014: return "Image is too big to save as TIFF";
        case 100015: return "DNG version is unsupported";
    }
}


NegativeProcessor* NegativeProcessor::createProcessor(AutoPtr<dng_host> &host, const char *filename) {
    // -----------------------------------------------------------------------------------------
    // Open and parse rawfile with libraw...

    AutoPtr<LibRaw> rawProcessor(new LibRaw());

    int ret = rawProcessor->open_file(filename);
    if (ret != LIBRAW_SUCCESS) {
        rawProcessor->recycle();
        std::stringstream error; error << "LibRaw-error while opening rawFile: " << libraw_strerror(ret);
        throw std::runtime_error(error.str());
    }

    ret = rawProcessor->unpack();
    if (ret != LIBRAW_SUCCESS) {
        rawProcessor->recycle();
        std::stringstream error; error << "LibRaw-error while unpacking rawFile: " << libraw_strerror(ret);
        throw std::runtime_error(error.str());
    }

    // -----------------------------------------------------------------------------------------
    // ...and libexiv2

    Exiv2::Image::AutoPtr rawImage;
    try {
        rawImage = Exiv2::ImageFactory::open(filename);
        rawImage->readMetadata();
    } 
    catch (Exiv2::Error& e) {
        std::stringstream error; error << "Exiv2-error while opening/parsing rawFile (code " << e.code() << "): " << e.what();
        throw std::runtime_error(error.str());
    }

    // -----------------------------------------------------------------------------------------
    // Identify and create correct processor class

    if (rawProcessor->imgdata.idata.dng_version != 0) {
        try {return new DNGprocessor(host, rawProcessor.Release(), rawImage);}
        catch (dng_exception &e) {
            std::stringstream error; error << "Cannot parse source DNG-file (" << e.ErrorCode() << ": " << getDngErrorMessage(e.ErrorCode()) << ")";
            throw std::runtime_error(error.str());
        }
    }
    else if (!strcmp(rawProcessor->imgdata.idata.model, "ILCE-7"))
        return new ILCE7processor(host, rawProcessor.Release(), rawImage);
    else if (!strcmp(rawProcessor->imgdata.idata.make, "FUJIFILM"))
        return new FujiProcessor(host, rawProcessor.Release(), rawImage);

    return new VariousVendorProcessor(host, rawProcessor.Release(), rawImage);
}


NegativeProcessor::NegativeProcessor(AutoPtr<dng_host> &host, LibRaw *rawProcessor, Exiv2::Image::AutoPtr &rawImage)
                                   : m_RawProcessor(rawProcessor), m_RawImage(rawImage),
                                     m_RawExif(m_RawImage->exifData()), m_RawXmp(m_RawImage->xmpData()),
                                     m_host(host) {
    m_negative.Reset(m_host->Make_dng_negative());
}


NegativeProcessor::~NegativeProcessor() {
	m_RawProcessor->recycle();
}


ColorKeyCode colorKey(const char color) {
    switch (color) {
        case 'R': return colorKeyRed;
        case 'G': return colorKeyGreen;
        case 'B': return colorKeyBlue;
        case 'C': return colorKeyCyan;
        case 'M': return colorKeyMagenta;
        case 'Y': return colorKeyYellow;
        case 'E': return colorKeyCyan; // only in the Sony DSC-F828. 'Emerald' - like cyan according to Sony
        case 'T':                      // what is 'T'??? LibRaw reports that only for the Leaf Catchlight, so I guess we're not compatible with early '90s tech...
        default:  return colorKeyMaxEnum;
    }
}


void NegativeProcessor::setDNGPropertiesFromRaw() {
    libraw_image_sizes_t *sizes   = &m_RawProcessor->imgdata.sizes;
    libraw_iparams_t     *iparams = &m_RawProcessor->imgdata.idata;

    // -----------------------------------------------------------------------------------------
    // Raw filename

    std::string file(m_RawImage->io().path());
    size_t found = std::min(file.rfind("\\"), file.rfind("/"));
    if (found != std::string::npos) file = file.substr(found + 1, file.length() - found - 1);
    m_negative->SetOriginalRawFileName(file.c_str());

	// -----------------------------------------------------------------------------------------
	// Model

    dng_string makeModel;
    makeModel.Append(iparams->make);
    makeModel.Append(" ");
    makeModel.Append(iparams->model);
    m_negative->SetModelName(makeModel.Get());

    // -----------------------------------------------------------------------------------------
    // Orientation 

    switch (sizes->flip) {
        case 180:
        case 3:  m_negative->SetBaseOrientation(dng_orientation::Rotate180()); break;
        case 270:
        case 5:  m_negative->SetBaseOrientation(dng_orientation::Rotate90CCW()); break;
        case 90:
        case 6:  m_negative->SetBaseOrientation(dng_orientation::Rotate90CW()); break;
        default: m_negative->SetBaseOrientation(dng_orientation::Normal()); break;
    }

	// -----------------------------------------------------------------------------------------
	// ColorKeys (this needs to happen before Mosaic - how about documenting that in the SDK???)

    m_negative->SetColorChannels(iparams->colors);
    m_negative->SetColorKeys(colorKey(iparams->cdesc[0]), colorKey(iparams->cdesc[1]), 
                             colorKey(iparams->cdesc[2]), colorKey(iparams->cdesc[3]));

    // -----------------------------------------------------------------------------------------
    // Mosaic

    if (iparams->colors == 4) m_negative->SetQuadMosaic(iparams->filters);
    else switch(iparams->filters) {
            case 0xe1e1e1e1:  m_negative->SetBayerMosaic(0); break;
            case 0xb4b4b4b4:  m_negative->SetBayerMosaic(1); break;
            case 0x1e1e1e1e:  m_negative->SetBayerMosaic(2); break;
            case 0x4b4b4b4b:  m_negative->SetBayerMosaic(3); break;
            default: break;  // not throwing error, because this might be set in a sub-class (e.g., Fuji)
        }

	// -----------------------------------------------------------------------------------------
	// Default scale and crop/active area

    m_negative->SetDefaultScale(dng_urational(sizes->iwidth, sizes->width), dng_urational(sizes->iheight, sizes->height));
    m_negative->SetActiveArea(dng_rect(sizes->top_margin, sizes->left_margin,
                                       sizes->top_margin + sizes->height, sizes->left_margin + sizes->width));

    uint32 cropWidth, cropHeight;
    if (!getRawExifTag("Exif.Photo.PixelXDimension", 0, &cropWidth) ||
        !getRawExifTag("Exif.Photo.PixelYDimension", 0, &cropHeight)) {
        cropWidth = sizes->width - 16;
        cropHeight = sizes->height - 16;
    }

    int cropLeftMargin = (cropWidth > sizes->width ) ? 0 : (sizes->width  - cropWidth) / 2;
    int cropTopMargin = (cropHeight > sizes->height) ? 0 : (sizes->height - cropHeight) / 2;

    m_negative->SetDefaultCropOrigin(cropLeftMargin, cropTopMargin);
    m_negative->SetDefaultCropSize(cropWidth, cropHeight);

    // -----------------------------------------------------------------------------------------
    // CameraNeutral

    //FIXME: what does this actually do?
    //FIXME: some pictures have 0 in cam_mul leading to NaNs; corrected here with 0-override but is that correct?
    dng_vector cameraNeutral(iparams->colors);
    for (int i = 0; i < iparams->colors; i++)
        cameraNeutral[i] = m_RawProcessor->imgdata.color.cam_mul[i] == 0 ? 0.0 : 1.0 / m_RawProcessor->imgdata.color.cam_mul[i];
    m_negative->SetCameraNeutral(cameraNeutral);

    // -----------------------------------------------------------------------------------------
    // BlackLevel & WhiteLevel

    libraw_colordata_t *colors  = &m_RawProcessor->imgdata.color;

    for (int i = 0; i < 4; i++)
	    m_negative->SetWhiteLevel(static_cast<uint32>(colors->maximum), i);

    if ((m_negative->GetMosaicInfo() != NULL) && (m_negative->GetMosaicInfo()->fCFAPatternSize == dng_point(2, 2)))
        m_negative->SetQuadBlacks(colors->black + colors->cblack[0],
                                  colors->black + colors->cblack[1],
                                  colors->black + colors->cblack[2],
                                  colors->black + colors->cblack[3]);
    else 
    	m_negative->SetBlackLevel(colors->black + colors->cblack[0], 0);

    // -----------------------------------------------------------------------------------------
    // Fixed properties

    m_negative->SetBaselineExposure(0.0);                       // should be fixed per camera
    m_negative->SetBaselineNoise(1.0);
    m_negative->SetBaselineSharpness(1.0);

    // default
    m_negative->SetAntiAliasStrength(dng_urational(100, 100));  // = no aliasing artifacts
    m_negative->SetLinearResponseLimit(1.0);                    // = no non-linear sensor response
    m_negative->SetAnalogBalance(dng_vector_3(1.0, 1.0, 1.0));
    m_negative->SetShadowScale(dng_urational(1, 1));
}


void NegativeProcessor::setCameraProfile(const char *dcpFilename) {
    AutoPtr<dng_camera_profile> prof(new dng_camera_profile);

    if (strlen(dcpFilename) > 0) {
        dng_file_stream profStream(dcpFilename);
        if (!prof->ParseExtended(profStream))
            throw std::runtime_error("Could not parse supplied camera profile file!");
    }
    else {
        // -----------------------------------------------------------------------------------------
        // Build our own minimal profile, based on one colour matrix provided by LibRaw

        dng_string profName;
        profName.Append(m_RawProcessor->imgdata.idata.make);
        profName.Append(" ");
        profName.Append(m_RawProcessor->imgdata.idata.model);

        prof->SetName(profName.Get());
        prof->SetCalibrationIlluminant1(lsD65);

        int colors = m_RawProcessor->imgdata.idata.colors;
        if ((colors == 3) || (colors = 4)) {
	        dng_matrix *colormatrix1 = new dng_matrix(colors, 3);

        	for (int i = 0; i < colors; i++)
        		for (int j = 0; j < 3; j++)
        			(*colormatrix1)[i][j] = m_RawProcessor->imgdata.color.cam_xyz[i][j];

            if (colormatrix1->MaxEntry() == 0.0) {
                printf("Warning, camera XYZ Matrix is null\n");
                delete colormatrix1;
                if (colors == 3)
                	colormatrix1 = new dng_matrix_3by3(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
                else
                	colormatrix1 = new dng_matrix_4by3(0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
            }
            prof->SetColorMatrix1(*colormatrix1);
        }
        prof->SetProfileCalibrationSignature("com.fimagena.raw2dng");
    }

    m_negative->AddProfile(prof);
}


void NegativeProcessor::setExifFromRaw(const dng_date_time_info &dateTimeNow, const dng_string &appNameVersion) {
    dng_exif *negExif = m_negative->GetExif();

    // -----------------------------------------------------------------------------------------
    // TIFF 6.0 "D. Other Tags"
    getRawExifTag("Exif.Image.DateTime", &negExif->fDateTime);
    getRawExifTag("Exif.Image.ImageDescription", &negExif->fImageDescription);
    getRawExifTag("Exif.Image.Make", &negExif->fMake);
    getRawExifTag("Exif.Image.Model", &negExif->fModel);
    getRawExifTag("Exif.Image.Software", &negExif->fSoftware);
    getRawExifTag("Exif.Image.Artist", &negExif->fArtist);
    getRawExifTag("Exif.Image.Copyright", &negExif->fCopyright);

    // -----------------------------------------------------------------------------------------
    // Exif 2.3 "A. Tags Relating to Version" (order as in spec)
    getRawExifTag("Exif.Photo.ExifVersion", 0, &negExif->fExifVersion);
    // Exif.Photo.FlashpixVersion - fFlashPixVersion : ignoring this here

    // -----------------------------------------------------------------------------------------
    // Exif 2.3 "B. Tags Relating to Image data Characteristics" (order as in spec)
    getRawExifTag("Exif.Photo.ColorSpace", 0, &negExif->fColorSpace);
    // Gamma : Supported by DNG SDK (fGamma) but not Exiv2 (v0.24)

    // -----------------------------------------------------------------------------------------
    // Exif 2.3 "C. Tags Relating To Image Configuration" (order as in spec)
    getRawExifTag("Exif.Photo.ComponentsConfiguration", 0, &negExif->fComponentsConfiguration);
    getRawExifTag("Exif.Photo.CompressedBitsPerPixel", 0, &negExif->fCompresssedBitsPerPixel);  // nice typo in DNG SDK...
    getRawExifTag("Exif.Photo.PixelXDimension", 0, &negExif->fPixelXDimension);
    getRawExifTag("Exif.Photo.PixelYDimension", 0, &negExif->fPixelYDimension);

    // -----------------------------------------------------------------------------------------
    // Exif 2.3 "D. Tags Relating to User Information" (order as in spec)
    // MakerNote: We'll deal with that below
    getRawExifTag("Exif.Photo.UserComment", &negExif->fUserComment);

    // -----------------------------------------------------------------------------------------
    // Exif 2.3 "E. Tags Relating to Related File Information" (order as in spec)
    // RelatedSoundFile : DNG SDK doesn't support this

    // -----------------------------------------------------------------------------------------
    // Exif 2.3 "F. Tags Relating to Date and Time" (order as in spec)
    getRawExifTag("Exif.Photo.DateTimeOriginal", &negExif->fDateTimeOriginal);
    getRawExifTag("Exif.Photo.DateTimeDigitized", &negExif->fDateTimeDigitized);
    // SubSecTime          : DNG SDK doesn't support this
    // SubSecTimeOriginal  : DNG SDK doesn't support this
    // SubSecTimeDigitized : DNG SDK doesn't support this

    // -----------------------------------------------------------------------------------------
    // Exif 2.3 "G. Tags Relating to Picture-Taking Conditions" (order as in spec)
    getRawExifTag("Exif.Photo.ExposureTime", 0, &negExif->fExposureTime);
    getRawExifTag("Exif.Photo.FNumber", 0, &negExif->fFNumber);
    getRawExifTag("Exif.Photo.ExposureProgram", 0, &negExif->fExposureProgram);
    // SpectralSensitivity : DNG SDK doesn't support this
    getRawExifTag("Exif.Photo.ISOSpeedRatings", negExif->fISOSpeedRatings, 3); // PhotographicSensitivity in Exif 2.3
    // OECF : DNG SDK doesn't support this
    getRawExifTag("Exif.Photo.SensitivityType", 0, &negExif->fSensitivityType);
    getRawExifTag("Exif.Photo.StandardOutputSensitivity", 0, &negExif->fStandardOutputSensitivity);
    getRawExifTag("Exif.Photo.RecommendedExposureIndex", 0, &negExif->fRecommendedExposureIndex);
    getRawExifTag("Exif.Photo.ISOSpeed", 0, &negExif->fISOSpeed);
    getRawExifTag("Exif.Photo.ISOSpeedLatitudeyyy", 0, &negExif->fISOSpeedLatitudeyyy);
    getRawExifTag("Exif.Photo.ISOSpeedLatitudezzz", 0, &negExif->fISOSpeedLatitudezzz);
    getRawExifTag("Exif.Photo.ShutterSpeedValue", 0, &negExif->fShutterSpeedValue);
    getRawExifTag("Exif.Photo.ApertureValue", 0, &negExif->fApertureValue);
    getRawExifTag("Exif.Photo.BrightnessValue", 0, &negExif->fBrightnessValue);
    getRawExifTag("Exif.Photo.ExposureBiasValue", 0, &negExif->fExposureBiasValue);
    getRawExifTag("Exif.Photo.MaxApertureValue", 0, &negExif->fMaxApertureValue);
    getRawExifTag("Exif.Photo.SubjectDistance", 0, &negExif->fSubjectDistance);
    getRawExifTag("Exif.Photo.MeteringMode", 0, &negExif->fMeteringMode);
    getRawExifTag("Exif.Photo.LightSource", 0, &negExif->fLightSource);
    getRawExifTag("Exif.Photo.Flash", 0, &negExif->fFlash);
    getRawExifTag("Exif.Photo.FocalLength", 0, &negExif->fFocalLength);
    negExif->fSubjectAreaCount = getRawExifTag("Exif.Photo.SubjectArea", negExif->fSubjectArea, 4);
    // FlashEnergy : DNG SDK doesn't support this
    // SpatialFrequencyResponse : DNG SDK doesn't support this
    getRawExifTag("Exif.Photo.FocalPlaneXResolution", 0, &negExif->fFocalPlaneXResolution);
    getRawExifTag("Exif.Photo.FocalPlaneYResolution", 0, &negExif->fFocalPlaneYResolution);
    getRawExifTag("Exif.Photo.FocalPlaneResolutionUnit", 0, &negExif->fFocalPlaneResolutionUnit);
    // SubjectLocation : DNG SDK doesn't support this
    getRawExifTag("Exif.Photo.ExposureIndex", 0, &negExif->fExposureIndex);
    getRawExifTag("Exif.Photo.SensingMethod", 0, &negExif->fSensingMethod);
    getRawExifTag("Exif.Photo.FileSource", 0, &negExif->fFileSource);
    getRawExifTag("Exif.Photo.SceneType", 0, &negExif->fSceneType);
    // CFAPattern: we write it manually from raw data further below
    getRawExifTag("Exif.Photo.CustomRendered", 0, &negExif->fCustomRendered);
    getRawExifTag("Exif.Photo.ExposureMode", 0, &negExif->fExposureMode);
    getRawExifTag("Exif.Photo.WhiteBalance", 0, &negExif->fWhiteBalance);
    getRawExifTag("Exif.Photo.DigitalZoomRatio", 0, &negExif->fDigitalZoomRatio);
    getRawExifTag("Exif.Photo.FocalLengthIn35mmFilm", 0, &negExif->fFocalLengthIn35mmFilm);
    getRawExifTag("Exif.Photo.SceneCaptureType", 0, &negExif->fSceneCaptureType);
    getRawExifTag("Exif.Photo.GainControl", 0, &negExif->fGainControl);
    getRawExifTag("Exif.Photo.Contrast", 0, &negExif->fContrast);
    getRawExifTag("Exif.Photo.Saturation", 0, &negExif->fSaturation);
    getRawExifTag("Exif.Photo.Sharpness", 0, &negExif->fSharpness);
    // DeviceSettingsDescription : DNG SDK doesn't support this
    getRawExifTag("Exif.Photo.SubjectDistanceRange", 0, &negExif->fSubjectDistanceRange);

    // -----------------------------------------------------------------------------------------
    // Exif 2.3 "H. Other Tags" (order as in spec)
    // ImageUniqueID : DNG SDK doesn't support this
    getRawExifTag("Exif.Photo.CameraOwnerName", &negExif->fOwnerName);
    getRawExifTag("Exif.Photo.BodySerialNumber", &negExif->fCameraSerialNumber);
    getRawExifTag("Exif.Photo.LensSpecification", negExif->fLensInfo, 4);
    getRawExifTag("Exif.Photo.LensMake", &negExif->fLensMake);
    getRawExifTag("Exif.Photo.LensModel", &negExif->fLensName);
    getRawExifTag("Exif.Photo.LensSerialNumber", &negExif->fLensSerialNumber);

    // -----------------------------------------------------------------------------------------
    // Exif 2.3 GPS "A. Tags Relating to GPS" (order as in spec)
    uint32 gpsVer[4];  gpsVer[0] = gpsVer[1] = gpsVer[2] = gpsVer[3] = 0;
    getRawExifTag("Exif.GPSInfo.GPSVersionID", gpsVer, 4);
    negExif->fGPSVersionID = (gpsVer[0] << 24) + (gpsVer[1] << 16) + (gpsVer[2] <<  8) + gpsVer[3];
    getRawExifTag("Exif.GPSInfo.GPSLatitudeRef", &negExif->fGPSLatitudeRef);
    getRawExifTag("Exif.GPSInfo.GPSLatitude", negExif->fGPSLatitude, 3);
    getRawExifTag("Exif.GPSInfo.GPSLongitudeRef", &negExif->fGPSLongitudeRef);
    getRawExifTag("Exif.GPSInfo.GPSLongitude", negExif->fGPSLongitude, 3);
    getRawExifTag("Exif.GPSInfo.GPSAltitudeRef", 0, &negExif->fGPSAltitudeRef);
    getRawExifTag("Exif.GPSInfo.GPSAltitude", 0, &negExif->fGPSAltitude);
    getRawExifTag("Exif.GPSInfo.GPSTimeStamp", negExif->fGPSTimeStamp, 3);
    getRawExifTag("Exif.GPSInfo.GPSSatellites", &negExif->fGPSSatellites);
    getRawExifTag("Exif.GPSInfo.GPSStatus", &negExif->fGPSStatus);
    getRawExifTag("Exif.GPSInfo.GPSMeasureMode", &negExif->fGPSMeasureMode);
    getRawExifTag("Exif.GPSInfo.GPSDOP", 0, &negExif->fGPSDOP);
    getRawExifTag("Exif.GPSInfo.GPSSpeedRef", &negExif->fGPSSpeedRef);
    getRawExifTag("Exif.GPSInfo.GPSSpeed", 0, &negExif->fGPSSpeed);
    getRawExifTag("Exif.GPSInfo.GPSTrackRef", &negExif->fGPSTrackRef);
    getRawExifTag("Exif.GPSInfo.GPSTrack", 0, &negExif->fGPSTrack);
    getRawExifTag("Exif.GPSInfo.GPSImgDirectionRef", &negExif->fGPSImgDirectionRef);
    getRawExifTag("Exif.GPSInfo.GPSImgDirection", 0, &negExif->fGPSImgDirection);
    getRawExifTag("Exif.GPSInfo.GPSMapDatum", &negExif->fGPSMapDatum);
    getRawExifTag("Exif.GPSInfo.GPSDestLatitudeRef", &negExif->fGPSDestLatitudeRef);
    getRawExifTag("Exif.GPSInfo.GPSDestLatitude", negExif->fGPSDestLatitude, 3);
    getRawExifTag("Exif.GPSInfo.GPSDestLongitudeRef", &negExif->fGPSDestLongitudeRef);
    getRawExifTag("Exif.GPSInfo.GPSDestLongitude", negExif->fGPSDestLongitude, 3);
    getRawExifTag("Exif.GPSInfo.GPSDestBearingRef", &negExif->fGPSDestBearingRef);
    getRawExifTag("Exif.GPSInfo.GPSDestBearing", 0, &negExif->fGPSDestBearing);
    getRawExifTag("Exif.GPSInfo.GPSDestDistanceRef", &negExif->fGPSDestDistanceRef);
    getRawExifTag("Exif.GPSInfo.GPSDestDistance", 0, &negExif->fGPSDestDistance);
    getRawExifTag("Exif.GPSInfo.GPSProcessingMethod", &negExif->fGPSProcessingMethod);
    getRawExifTag("Exif.GPSInfo.GPSAreaInformation", &negExif->fGPSAreaInformation);
    getRawExifTag("Exif.GPSInfo.GPSDateStamp", &negExif->fGPSDateStamp);
    getRawExifTag("Exif.GPSInfo.GPSDifferential", 0, &negExif->fGPSDifferential);
    // GPSHPositioningError : Supported by DNG SDK (fGPSHPositioningError) but not Exiv2 (v0.24)

    // -----------------------------------------------------------------------------------------
    // Exif 2.3, Interoperability IFD "A. Attached Information Related to Interoperability"
    getRawExifTag("Exif.Iop.InteroperabilityIndex", &negExif->fInteroperabilityIndex);
    getRawExifTag("Exif.Iop.InteroperabilityVersion", 0, &negExif->fInteroperabilityVersion); // this is not in the Exif standard but in DNG SDK and Exiv2

/*  Fields in the DNG SDK Exif structure that we are ignoring here. Some could potentially be 
    read through Exiv2 but are not part of the Exif standard so we leave them out:
     - fBatteryLevelA, fBatteryLevelR
     - fSelfTimerMode
     - fTIFF_EP_StandardID, fImageNumber
     - fApproxFocusDistance
     - fFlashCompensation, fFlashMask
     - fFirmware 
     - fLensID
     - fLensNameWasReadFromExif (--> available but not used by SDK)
     - fRelatedImageFileFormat, fRelatedImageWidth, fRelatedImageLength */

    // -----------------------------------------------------------------------------------------
    // Write CFAPattern (section G) manually from mosaicinfo

    const dng_mosaic_info* mosaicinfo = m_negative->GetMosaicInfo();
    if (mosaicinfo != NULL) {
        negExif->fCFARepeatPatternCols = mosaicinfo->fCFAPatternSize.v;
        negExif->fCFARepeatPatternRows = mosaicinfo->fCFAPatternSize.h;
        for (uint16 c = 0; c < negExif->fCFARepeatPatternCols; c++)
            for (uint16 r = 0; r < negExif->fCFARepeatPatternRows; r++)
                negExif->fCFAPattern[r][c] = mosaicinfo->fCFAPattern[c][r];
    }

    // -----------------------------------------------------------------------------------------
    // Reconstruct LensName from LensInfo if not present

    if (negExif->fLensName.IsEmpty()) {
        dng_urational *li = negExif->fLensInfo;
        std::stringstream lensName; lensName.precision(1); lensName.setf(std::ios::fixed, std::ios::floatfield);

        if (li[0].IsValid())      lensName << li[0].As_real64();
        if (li[1] != li[2])       lensName << "-" << li[1].As_real64();
        if (lensName.tellp() > 0) lensName << " mm";
        if (li[2].IsValid())      lensName << " f/" << li[2].As_real64();
        if (li[3] != li[2])       lensName << "-" << li[3].As_real64();

        negExif->fLensName.Set_ASCII(lensName.str().c_str());
    }

    // -----------------------------------------------------------------------------------------
    // Overwrite date, software, version - these are now referencing the DNG-creation

    negExif->fDateTime = dateTimeNow;
    negExif->fSoftware = appNameVersion;
    negExif->fExifVersion = DNG_CHAR4 ('0','2','3','0'); 
}


void NegativeProcessor::setXmpFromRaw(const dng_date_time_info &dateTimeNow, const dng_string &appNameVersion) {
    // -----------------------------------------------------------------------------------------
    // Copy existing XMP-tags in raw-file to DNG

    AutoPtr<dng_xmp> negXmp(new dng_xmp(m_host->Allocator()));
    for (Exiv2::XmpData::const_iterator it = m_RawXmp.begin(); it != m_RawXmp.end(); it++) {
        try {
            negXmp->Set(Exiv2::XmpProperties::nsInfo(it->groupName())->ns_, it->tagName().c_str(), it->toString().c_str());
        }
        catch (dng_exception& e) {
            // the above will throw an exception when trying to add XMPs with unregistered (i.e., unknown) 
            // namespaces -- we just drop them here.
            std::cerr << "Dropped XMP-entry from raw-file since namespace is unknown: "
                         "NS: "   << Exiv2::XmpProperties::nsInfo(it->groupName())->ns_ << ", "
                         "path: " << it->tagName().c_str() << ", "
                         "text: " << it->toString().c_str() << "\n";
        }
    }

    // -----------------------------------------------------------------------------------------
    // Set some base-XMP tags (incl. redundant creation date under Photoshop namespace - just to stay close to Adobe...)

    negXmp->UpdateDateTime(dateTimeNow);
    negXmp->UpdateMetadataDate(dateTimeNow);
    negXmp->SetString(XMP_NS_XAP, "CreatorTool", appNameVersion);
    negXmp->Set(XMP_NS_DC, "format", "image/dng");
    negXmp->SetString(XMP_NS_PHOTOSHOP, "DateCreated", m_negative->GetExif()->fDateTimeOriginal.Encode_ISO_8601());

    m_negative->ResetXMP(negXmp.Release());
}


void NegativeProcessor::backupProprietaryData() {
    AutoPtr<dng_memory_stream> DNGPrivateTag(createDNGPrivateTag());

    if (DNGPrivateTag.Get()) {
        AutoPtr<dng_memory_block> blockPriv(DNGPrivateTag->AsMemoryBlock(m_host->Allocator()));
        m_negative->SetPrivateData(blockPriv);
    }
}


dng_memory_stream* NegativeProcessor::createDNGPrivateTag() {
    uint32 mnOffset = 0;
    dng_string mnByteOrder;
    long mnLength = 0;
    unsigned char* mnBuffer = 0;

    if (getRawExifTag("Exif.MakerNote.Offset", 0, &mnOffset) &&
        getRawExifTag("Exif.MakerNote.ByteOrder", &mnByteOrder) &&
        getRawExifTag("Exif.Photo.MakerNote", &mnLength, &mnBuffer)) {
        bool padding = (mnLength & 0x01) == 0x01;

        dng_memory_stream *streamPriv = new dng_memory_stream(m_host->Allocator());
        streamPriv->SetBigEndian();

        // -----------------------------------------------------------------------------------------
        // Use Adobe vendor-format for encoding MakerNotes in DNGPrivateTag

        streamPriv->Put("Adobe", 5);
        streamPriv->Put_uint8(0x00);

        streamPriv->Put("MakN", 4);
        streamPriv->Put_uint32(mnLength + mnByteOrder.Length() + 4 + (padding ? 1 : 0));
        streamPriv->Put(mnByteOrder.Get(), mnByteOrder.Length());
        streamPriv->Put_uint32(mnOffset);
        streamPriv->Put(mnBuffer, mnLength);
        if (padding) streamPriv->Put_uint8(0x00);

        delete[] mnBuffer;

        return streamPriv;
    }

    return NULL;
}


void NegativeProcessor::buildDNGImage() {
    libraw_image_sizes_t *sizes = &m_RawProcessor->imgdata.sizes;

    // -----------------------------------------------------------------------------------------
    // Select right data source from LibRaw

    unsigned short *rawBuffer = (unsigned short*) m_RawProcessor->imgdata.rawdata.raw_image;
    uint32 inputPlanes = 1;

    if (rawBuffer == NULL) {
        rawBuffer = (unsigned short*) m_RawProcessor->imgdata.rawdata.color3_image;
        inputPlanes = 3;
    }
    if (rawBuffer == NULL) {
        rawBuffer = (unsigned short*) m_RawProcessor->imgdata.rawdata.color4_image;
        inputPlanes = 4;
    }

    uint32 outputPlanes = (inputPlanes == 1) ? 1 : m_RawProcessor->imgdata.idata.colors;

    // -----------------------------------------------------------------------------------------
    // Create new dng_image and copy data

    dng_rect bounds = dng_rect(sizes->raw_height, sizes->raw_width);
    dng_simple_image *image = new dng_simple_image(bounds, outputPlanes, ttShort, m_host->Allocator());

    dng_pixel_buffer buffer; image->GetPixelBuffer(buffer);
    unsigned short *imageBuffer = (unsigned short*)buffer.fData;

    if (inputPlanes == outputPlanes)
        memcpy(imageBuffer, rawBuffer, sizes->raw_height * sizes->raw_width * outputPlanes * sizeof(unsigned short));
    else {
        for (int i = 0; i < (sizes->raw_height * sizes->raw_width); i++) {
            memcpy(imageBuffer, rawBuffer, outputPlanes * sizeof(unsigned short));
            imageBuffer += outputPlanes;
            rawBuffer += inputPlanes;
        }
    }

    AutoPtr<dng_image> castImage(dynamic_cast<dng_image*>(image));
    m_negative->SetStage1Image(castImage);
}


void NegativeProcessor::embedOriginalRaw(const char *rawFilename) {
    #define BLOCKSIZE 65536 // as per spec

    // -----------------------------------------------------------------------------------------
    // Open input/output streams and write header with empty indices

    dng_file_stream rawDataStream(rawFilename);
    rawDataStream.SetReadPosition(0);

    uint32 rawFileSize = static_cast<uint32>(rawDataStream.Length());
    uint32 numberRawBlocks = static_cast<uint32>(floor((rawFileSize + 65535.0) / 65536.0));

    dng_memory_stream embeddedRawStream(m_host->Allocator());
    embeddedRawStream.SetBigEndian(true);
    embeddedRawStream.Put_uint32(rawFileSize);
    for (uint32 block = 0; block < numberRawBlocks; block++) 
        embeddedRawStream.Put_uint32(0);  // indices for the block-offsets
    embeddedRawStream.Put_uint32(0);  // index to next data fork

    uint32 indexOffset = 1 * sizeof(uint32);
    uint32 dataOffset = (numberRawBlocks + 1 + 1) * sizeof(uint32);

    for (uint32 block = 0; block < numberRawBlocks; block++) {

        // -----------------------------------------------------------------------------------------
        // Read and compress one 64k block of data

        z_stream zstrm;
        zstrm.zalloc = Z_NULL;
        zstrm.zfree = Z_NULL;
        zstrm.opaque = Z_NULL;
        if (deflateInit(&zstrm, Z_DEFAULT_COMPRESSION) != Z_OK) 
            throw std::runtime_error("Error initialising ZLib for embedding raw file!");

        unsigned char inBuffer[BLOCKSIZE], outBuffer[BLOCKSIZE * 2];
        uint32 currentRawBlockLength = 
            static_cast<uint32>(std::min(static_cast<uint64>(BLOCKSIZE), rawFileSize - rawDataStream.Position()));
        rawDataStream.Get(inBuffer, currentRawBlockLength);
        zstrm.avail_in = currentRawBlockLength;
        zstrm.next_in = inBuffer;
        zstrm.avail_out = BLOCKSIZE * 2;
        zstrm.next_out = outBuffer;
        if (deflate(&zstrm, Z_FINISH) != Z_STREAM_END)
            throw std::runtime_error("Error compressing chunk for embedding raw file!");

        uint32 compressedBlockLength = zstrm.total_out;
        deflateEnd(&zstrm);

        // -----------------------------------------------------------------------------------------
        // Write index and data

        embeddedRawStream.SetWritePosition(indexOffset);
        embeddedRawStream.Put_uint32(dataOffset);
        indexOffset += sizeof(uint32);

        embeddedRawStream.SetWritePosition(dataOffset);
        embeddedRawStream.Put(outBuffer, compressedBlockLength);
        dataOffset += compressedBlockLength;
    }

    embeddedRawStream.SetWritePosition(indexOffset);
    embeddedRawStream.Put_uint32(dataOffset);

    // -----------------------------------------------------------------------------------------
    // Write 7 "Mac OS forks" as per spec - empty for us

    embeddedRawStream.SetWritePosition(dataOffset);
    embeddedRawStream.Put_uint32(0);
    embeddedRawStream.Put_uint32(0);
    embeddedRawStream.Put_uint32(0);
    embeddedRawStream.Put_uint32(0);
    embeddedRawStream.Put_uint32(0);
    embeddedRawStream.Put_uint32(0);
    embeddedRawStream.Put_uint32(0);

    AutoPtr<dng_memory_block> block(embeddedRawStream.AsMemoryBlock(m_host->Allocator()));
    m_negative->SetOriginalRawFileData(block);
    m_negative->FindOriginalRawFileDigest();
}


// -----------------------------------------------------------------------------------------
// Protected helper functions

bool NegativeProcessor::getInterpretedRawExifTag(const char* exifTagName, int32 component, uint32* value) {
    Exiv2::ExifData::const_iterator it = m_RawExif.findKey(Exiv2::ExifKey(exifTagName));
    if (it == m_RawExif.end()) return false;

    std::stringstream interpretedValue; it->write(interpretedValue, &m_RawExif);

    uint32 tmp;
    for (int i = 0; (i <= component) && !interpretedValue.fail(); i++) interpretedValue >> tmp;
    if (interpretedValue.fail()) return false;

    *value = tmp;
    return true;
}

bool NegativeProcessor::getRawExifTag(const char* exifTagName, dng_string* value) {
    Exiv2::ExifData::const_iterator it = m_RawExif.findKey(Exiv2::ExifKey(exifTagName));
    if (it == m_RawExif.end()) return false;

    value->Set_ASCII((it->print(&m_RawExif)).c_str()); value->TrimLeadingBlanks(); value->TrimTrailingBlanks();
    return true;
}

bool NegativeProcessor::getRawExifTag(const char* exifTagName, dng_date_time_info* value) {
    Exiv2::ExifData::const_iterator it = m_RawExif.findKey(Exiv2::ExifKey(exifTagName));
    if (it == m_RawExif.end()) return false;

    dng_date_time dt; dt.Parse((it->print(&m_RawExif)).c_str()); value->SetDateTime(dt);
    return true;
}

bool NegativeProcessor::getRawExifTag(const char* exifTagName, int32 component, dng_srational* rational) {
    Exiv2::ExifData::const_iterator it = m_RawExif.findKey(Exiv2::ExifKey(exifTagName));
    if ((it == m_RawExif.end()) || (it->count() < (component + 1))) return false;

    Exiv2::Rational exiv2Rational = (*it).toRational(component);
    *rational = dng_srational(exiv2Rational.first, exiv2Rational.second);
    return true;
}

bool NegativeProcessor::getRawExifTag(const char* exifTagName, int32 component, dng_urational* rational) {
    Exiv2::ExifData::const_iterator it = m_RawExif.findKey(Exiv2::ExifKey(exifTagName));
    if ((it == m_RawExif.end()) || (it->count() < (component + 1))) return false;

    Exiv2::URational exiv2Rational = (*it).toRational(component);
    *rational = dng_urational(exiv2Rational.first, exiv2Rational.second);
    return true;
}

bool NegativeProcessor::getRawExifTag(const char* exifTagName, int32 component, uint32* value) {
    Exiv2::ExifData::const_iterator it = m_RawExif.findKey(Exiv2::ExifKey(exifTagName));
    if ((it == m_RawExif.end()) || (it->count() < (component + 1))) return false;

    *value = static_cast<uint32>(it->toLong(component));
    return true;
}

int NegativeProcessor::getRawExifTag(const char* exifTagName, uint32* valueArray, int32 maxFill) {
    Exiv2::ExifData::const_iterator it = m_RawExif.findKey(Exiv2::ExifKey(exifTagName));
    if (it == m_RawExif.end()) return 0;

    int lengthToFill = std::min(maxFill, static_cast<int32>(it->count()));
    for (int i = 0; i < lengthToFill; i++)
        valueArray[i] = static_cast<uint32>(it->toLong(i));
    return lengthToFill;
}

int NegativeProcessor::getRawExifTag(const char* exifTagName, int16* valueArray, int32 maxFill) {
    Exiv2::ExifData::const_iterator it = m_RawExif.findKey(Exiv2::ExifKey(exifTagName));
    if (it == m_RawExif.end()) return 0;

    int lengthToFill = std::min(maxFill, static_cast<int32>(it->count()));
    for (int i = 0; i < lengthToFill; i++)
        valueArray[i] = static_cast<int16>(it->toLong(i));
    return lengthToFill;
}

int NegativeProcessor::getRawExifTag(const char* exifTagName, dng_urational* valueArray, int32 maxFill) {
    Exiv2::ExifData::const_iterator it = m_RawExif.findKey(Exiv2::ExifKey(exifTagName));
    if (it == m_RawExif.end()) return 0;

    int lengthToFill = std::min(maxFill, static_cast<int32>(it->count()));
    for (int i = 0; i < lengthToFill; i++) {
        Exiv2::URational exiv2Rational = (*it).toRational(i);
        valueArray[i] = dng_urational(exiv2Rational.first, exiv2Rational.second);
    }
    return lengthToFill;
}

bool NegativeProcessor::getRawExifTag(const char* exifTagName, long* size, unsigned char** data) {
    Exiv2::ExifData::const_iterator it = m_RawExif.findKey(Exiv2::ExifKey(exifTagName));
    if (it == m_RawExif.end()) return false;

    *data = new unsigned char[(*it).size()]; *size = (*it).size();
    (*it).copy((Exiv2::byte*)*data, Exiv2::bigEndian);
    return true;
}

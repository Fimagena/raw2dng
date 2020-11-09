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

#pragma once

#include <dng_host.h>
#include <dng_negative.h>
#include <dng_exif.h>
#include <exiv2/image.hpp>

class LibRaw;

const char* getDngErrorMessage(int errorCode);

class NegativeProcessor {
public:
   static NegativeProcessor* createProcessor(AutoPtr<dng_host> &host, const char *filename);
   virtual ~NegativeProcessor();

   dng_negative* getNegative() {return m_negative.Get();}

   // Different raw/DNG processing stages - usually called in this sequence
   virtual void setDNGPropertiesFromRaw();
   virtual void setCameraProfile(const char *dcpFilename);
   virtual void setExifFromRaw(const dng_date_time_info &dateTimeNow, const dng_string &appNameVersion);
   virtual void setXmpFromRaw(const dng_date_time_info &dateTimeNow, const dng_string &appNameVersion);
   virtual void backupProprietaryData();
   virtual void buildDNGImage();
   virtual void embedOriginalRaw(const char *rawFilename);

protected:
   NegativeProcessor(AutoPtr<dng_host> &host, LibRaw *rawProcessor, Exiv2::Image::UniquePtr &rawImage);

   virtual dng_memory_stream* createDNGPrivateTag();

   // helper functions
   bool getInterpretedRawExifTag(const char* exifTagName, int32 component, uint32* value);

   bool getRawExifTag(const char* exifTagName, dng_string* value);
   bool getRawExifTag(const char* exifTagName, dng_date_time_info* value);
   bool getRawExifTag(const char* exifTagName, int32 component, dng_srational* rational);
   bool getRawExifTag(const char* exifTagName, int32 component, dng_urational* rational);
   bool getRawExifTag(const char* exifTagName, int32 component, uint32* value);

   int  getRawExifTag(const char* exifTagName, uint32* valueArray, int32 maxFill);
   int  getRawExifTag(const char* exifTagName, int16* valueArray, int32 maxFill);
   int  getRawExifTag(const char* exifTagName, dng_urational* valueArray, int32 maxFill);

   bool getRawExifTag(const char* exifTagName, long* size, unsigned char** data);

   // Source: Raw-file
   AutoPtr<LibRaw> m_RawProcessor;
   Exiv2::Image::UniquePtr m_RawImage;
   Exiv2::ExifData m_RawExif;
   Exiv2::XmpData m_RawXmp;

   // Target: DNG-file
   AutoPtr<dng_host> &m_host;
   AutoPtr<dng_negative> m_negative;
};

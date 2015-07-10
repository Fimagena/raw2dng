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

#include "../negativeProcessor.h"


// TODO/FIXME: Fuji support is currently broken!

class FujiProcessor : public NegativeProcessor {
friend class NegativeProcessor;

public:
   void setDNGPropertiesFromRaw();
   dng_image* buildDNGImage();

protected:
   FujiProcessor(AutoPtr<dng_host> &host, AutoPtr<dng_negative> &negative, 
                 LibRaw *rawProcessor, Exiv2::Image::AutoPtr &rawImage);

   bool m_fujiRotate90;
};

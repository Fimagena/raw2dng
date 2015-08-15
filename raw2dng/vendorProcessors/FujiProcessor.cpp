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

#include <dng_simple_image.h>

#include <libraw/libraw.h>

#include "FujiProcessor.h"


// TODO/FIXME: Fuji support is currently broken!

FujiProcessor::FujiProcessor(AutoPtr<dng_host> &host, LibRaw *rawProcessor, Exiv2::Image::AutoPtr &rawImage)
                           : NegativeProcessor(host, rawProcessor, rawImage) {
    m_fujiRotate90 = (2 == m_RawProcessor->COLOR(0, 1)) && (1 == m_RawProcessor->COLOR(1, 0));
}


void FujiProcessor::setDNGPropertiesFromRaw() {
    NegativeProcessor::setDNGPropertiesFromRaw();

    libraw_image_sizes_t *sizes   = &m_RawProcessor->imgdata.sizes;
    libraw_iparams_t     *iparams = &m_RawProcessor->imgdata.idata;

    // -----------------------------------------------------------------------------------------
    // Orientation 

    if (m_fujiRotate90) m_negative->SetBaseOrientation(m_negative->BaseOrientation() + dng_orientation::Mirror90CCW());

    // -----------------------------------------------------------------------------------------
    // Mosaic

    if (iparams->colors != 4) m_negative->SetFujiMosaic(0);

    // -----------------------------------------------------------------------------------------
    // Default scale and crop/active area

    if (m_fujiRotate90) {
        m_negative->SetDefaultScale(dng_urational(sizes->iheight, sizes->height), dng_urational(sizes->iwidth, sizes->width));
        m_negative->SetActiveArea(dng_rect(sizes->top_margin, sizes->left_margin,
                                           sizes->top_margin + sizes->width, sizes->left_margin + sizes->height));

        if (iparams->filters != 0) {
            m_negative->SetDefaultCropOrigin(8, 8);
            m_negative->SetDefaultCropSize(sizes->height - 16, sizes->width - 16);
        }
        else {
            m_negative->SetDefaultCropOrigin(0, 0);
            m_negative->SetDefaultCropSize(sizes->height, sizes->width);
        }
    }
}


void FujiProcessor::buildDNGImage() {
    NegativeProcessor::buildDNGImage();

// TODO: FIXME
/*    if (m_fujiRotate90) {
        dng_rect rotatedRect(dngImage->fBounds.W(), dngImage->fBounds.H());
        dngImage->fBounds = rotatedRect;
    }*/
}


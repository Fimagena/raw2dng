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

FujiProcessor::FujiProcessor(AutoPtr<dng_host> &host, AutoPtr<dng_negative> &negative, 
                             LibRaw *rawProcessor, Exiv2::Image::AutoPtr &rawImage)
                           : NegativeProcessor(host, negative, rawProcessor, rawImage) {
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
        uint32 outputWidth  = sizes->iheight;
        uint32 outputHeight = sizes->iwidth;
        uint32 visibleWidth  = sizes->height;
        uint32 visibleHeight = sizes->width;

        m_negative->SetDefaultScale(dng_urational(outputWidth, visibleWidth), 
                                    dng_urational(outputHeight, visibleHeight));

        if (iparams->filters != 0) {
            m_negative->SetDefaultCropOrigin(dng_urational(8, 1), dng_urational(8, 1));
            m_negative->SetDefaultCropSize(dng_urational(visibleWidth - 16, 1), dng_urational(visibleHeight - 16, 1));
        }
        else {
            m_negative->SetDefaultCropOrigin(dng_urational(0, 1), dng_urational(0, 1));
            m_negative->SetDefaultCropSize(dng_urational(visibleWidth, 1), dng_urational(visibleHeight, 1));
        }

        m_negative->SetActiveArea(dng_rect(sizes->top_margin,
                                           sizes->left_margin,
                                           sizes->top_margin + visibleHeight,
                                           sizes->left_margin + visibleWidth));
    }
}


dng_image* FujiProcessor::buildDNGImage() {
    if (!m_fujiRotate90) return NegativeProcessor::buildDNGImage();

    libraw_image_sizes_t *sizes = &m_RawProcessor->imgdata.sizes;

    // -----------------------------------------------------------------------------------------
    // Create new dng_image with right dimensions // this section is mostly copied from NegativeProcessor

    uint32 rawWidth  = std::max(static_cast<uint32>(sizes->raw_width),  static_cast<uint32>(sizes->width  + sizes->left_margin));
    uint32 rawHeight = std::max(static_cast<uint32>(sizes->raw_height), static_cast<uint32>(sizes->height + sizes->top_margin ));
    dng_rect bounds = dng_rect(rawWidth, rawHeight); // CHANGE from negativeProcessor

    int planes = (m_RawProcessor->imgdata.idata.filters == 0) ? 3 : 1;

    dng_simple_image *image = new dng_simple_image(bounds, planes, ttShort, m_host->Allocator());

    dng_pixel_buffer buffer; image->GetPixelBuffer(buffer);
    unsigned short *imageBuffer = (unsigned short*)buffer.fData;

    // -----------------------------------------------------------------------------------------
    // Select right data source and copy sensor data from raw-file to DNG-image

    unsigned short *rawBuffer = m_RawProcessor->imgdata.rawdata.raw_image;
    if (rawBuffer == NULL) rawBuffer = (unsigned short*) m_RawProcessor->imgdata.rawdata.color3_image;
    if (rawBuffer == NULL) rawBuffer = (unsigned short*) m_RawProcessor->imgdata.rawdata.color4_image;

    unsigned int colors = m_RawProcessor->imgdata.idata.colors;
    if (rawBuffer == m_RawProcessor->imgdata.rawdata.raw_image) colors = 1; // this is probably not necessary, need to check LibRaw sources

    for (unsigned int col = 0; col < sizes->raw_width; col++)
        for (unsigned int row = 0; row < sizes->raw_height; row++)
            for (uint32 color = 0; color < colors; color++) {
                *imageBuffer = rawBuffer[(row * sizes->raw_width + col) * colors + color];
                ++imageBuffer;
            }

    return dynamic_cast<dng_image*>(image);
}

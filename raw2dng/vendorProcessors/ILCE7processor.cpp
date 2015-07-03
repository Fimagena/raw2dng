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

#include "ILCE7processor.h"

#include <exception>
#include <stdexcept>

#include <dng_xmp.h>
#include <dng_lens_correction.h>
#include <dng_memory_stream.h>
#include <dng_camera_profile.h>

#include <libraw/libraw.h>
#include <exiv2/tiffimage.hpp>

static unsigned int ISOlist[28] = {50, 64, 80, 100, 125, 160, 200, 
                                   250, 320, 400, 500, 640, 800, 1000, 
                                   1250, 1600, 2000, 2500, 3200, 4000, 5000, 
                                   6400, 8000, 10000, 12800, 16000, 20000, 25600};

static uint64 NoiseProfileScalePerPlane[28][3] = {
{0x3ef391e4e020ad57, 0x3ef3a0a3ce83db9f, 0x3ef3eda556180639},
{0x3ef880b5ca074cb1, 0x3ef89cf912f01012, 0x3ef8f01f7dd8389f},
{0x3efecb4829e644ef, 0x3efefa62f8bf8a91, 0x3eff65b415032287},
{0x3ef37467035a50c4, 0x3ef39dc5cad420d8, 0x3ef3eb990a515c3d},
{0x3ef868f3f4a065ac, 0x3ef8701efdd7b89f, 0x3ef8d3f66bec9759},
{0x3eff58ec799c1c8c, 0x3eff3035787659e8, 0x3effb2df8e2c507d},
{0x3f03a34d64395233, 0x3f0373954b713ff9, 0x3f03c687485f2454},
{0x3f086067050fc664, 0x3f0839979c4e4acc, 0x3f089ad2902c43fd},
{0x3f0f02be196f35db, 0x3f0ee867a71d59f2, 0x3f0f5da25b183d1c},
{0x3f134bd9f3c9917c, 0x3f1345cf4772b5a2, 0x3f138bda33c9d17c},
{0x3f17cf11aa347b19, 0x3f17bcbd3735a359, 0x3f180dbd1f59ffd0},
{0x3f1e205fa996f55d, 0x3f1dfcd72079bcf2, 0x3f1e5d2e02bd73df},
{0x3f22ac5c66ba9bfa, 0x3f2290c3833f363e, 0x3f22c9b2f1054566},
{0x3f27436e8219b894, 0x3f271f940d257598, 0x3f2758fcb93e4276},
{0x3f2d004524509c54, 0x3f2cd218b98544c9, 0x3f2d0c18f3857ec9},
{0x3f3284526a1b8731, 0x3f326602d56c19d3, 0x3f32836d08f49cd3},
{0x3f370d19eb1abcd2, 0x3f36de352dab8d85, 0x3f370864099e9c43},
{0x3f3cb8134c59bfdc, 0x3f3c74741bfade22, 0x3f3cae98ca731b8d},
{0x3f4253b8370c2edb, 0x3f4223661b34f44c, 0x3f424ba485ce4d48},
{0x3f4696e43568198e, 0x3f4677cd8f267140, 0x3f4689a72062b92b},
{0x3f4beadb335afeed, 0x3f4be14ee0144d71, 0x3f4bd72a619c4006},
{0x3f51b034183decd3, 0x3f51ba81f57d740e, 0x3f51a1d7780ffe69},
{0x3f5607040147ffa4, 0x3f5629382866714f, 0x3f55fc4723309c06},
{0x3f5b7387e494972b, 0x3f5bb39be809ade0, 0x3f5b6d52b9196109},
{0x3f6185b9de331c0c, 0x3f61baad6090b489, 0x3f6185cb12493a6d},
{0x3f6328a9128ac314, 0x3f6357da68a59205, 0x3f63331679f2d757},
{0x3f65345413f853de, 0x3f655c52b2bfa6df, 0x3f654bb4bb86db7a},
{0x3f681176af91b82c, 0x3f682f6180e42a77, 0x3f683af8b0efae12}};

static uint64 NoiseProfileOffsetPerPlane[28][3] = {
{0x3e36f87fdc6ddf14, 0x3e371ee68fa1857b, 0x3e3775f17b27a954},
{0x3e3efdae9f154ab8, 0x3e3fca7d057e1788, 0x3e3f94ba0a9c2e93},
{0x3e458f8747eec2e6, 0x3e452690b372a6bb, 0x3e44a7d76415019c},
{0x3e376405d231b102, 0x3e37402e8045c05d, 0x3e37336199df3391},
{0x3e3eeb8d83ba971c, 0x3e3ed7a5a3bbf9e0, 0x3e3eafa8e1a377b1},
{0x3e459c9ad5f75b4f, 0x3e45a280a468b142, 0x3e4575ea32877a8d},
{0x3e4de6a2338ce6de, 0x3e4e055ac2e96bfd, 0x3e4db4b64a968e8a},
{0x3e54d95b95c837af, 0x3e550f6fcba2c67c, 0x3e54d1c512146da4},
{0x3e5ec048edbf0e8f, 0x3e5f3f35c87e31bd, 0x3e5edc627908e224},
{0x3e66354b22d1fbdf, 0x3e66abb276069247, 0x3e665fdad41a29a1},
{0x3e706af5cbb79497, 0x3e70c5f44d492923, 0x3e707862e052e93c},
{0x3e7998d2bbb5f6e9, 0x3e7a2babc8e84432, 0x3e7993e06fa92cbe},
{0x3e834959c5d37580, 0x3e83bacb4c8d1bc0, 0x3e83371c30b48675},
{0x3e8de1eaaa4de73f, 0x3e8e3adce7eb1c78, 0x3e8d8497bdeddf0c},
{0x3e9730378e110ecf, 0x3e973f33c9548461, 0x3e96bf42551e3b14},
{0x3ea2e22ae2fb35b8, 0x3ea2c7c9668a42bb, 0x3ea269342e7ee98c},
{0x3ead2bc16abd9e94, 0x3ead5cb8dbdd5706, 0x3eacdf352de27015},
{0x3eb694f068158d82, 0x3eb6f34979f2b2cc, 0x3eb69f2cd9166c69},
{0x3ec259be4d03d9e3, 0x3ec2cf13b28f7752, 0x3ec2942f65bcdb21},
{0x3eccc6ee0b0fc80d, 0x3ecd8e03930102c7, 0x3eccb7bef1e0ae4e},
{0x3ed68c21e403a4ce, 0x3ed7318cfdf6f541, 0x3ed63dec3fcc5595},
{0x3ee28487ea44fe16, 0x3ee3132678256243, 0x3ee2150852f4536f},
{0x3eecedba487a664a, 0x3eedc2219d9b2dd8, 0x3eec401f57de93de},
{0x3ef698e13fb11cea, 0x3ef7384d046b425c, 0x3ef611a1f3f48144},
{0x3f02825e37dd208f, 0x3f03004bae98d7c7, 0x3f0213d8dff1978f},
{0x3f089269582d1301, 0x3f0931e752709ac5, 0x3f080ce4beb68d35},
{0x3f10ad92ba71d1bd, 0x3f1114f978264709, 0x3f105b4d653c3062},
{0x3f17f3ca1071efa9, 0x3f18814e546facb0, 0x3f1789ad27bda716}};

static real64 CACorrectionBaseFunctions[16][4] = {
    { 0.0000000000000013, -0.0000000000000027,  0.0000000000000036,  0.0000000000000000},
    { 0.0000021859248025, -0.0000137421208786,  0.0000243311238322, -0.0000128811832028},
    { 0.0000080153067312, -0.0000495148646946,  0.0000868174909758, -0.0000456794900128},
    { 0.0000154765246907, -0.0000923728570994,  0.0001588057545483, -0.0000825078502622},
    { 0.0000217717045237, -0.0001219220778106,  0.0002016732933257, -0.0001021326679069},
    { 0.0000240660335060, -0.0001183805879963,  0.0001790368916001, -0.0000849637462483},
    { 0.0000203709466251, -0.0000696401924296,  0.0000708633798521, -0.0000210854709835},
    { 0.0000103953306270,  0.0000220943197666, -0.0001134992747893,  0.0000824254611747},
    {-0.00000384849505  ,  0.0001359183851202, -0.0003272037997810,  0.0001971518269581},
    {-0.00001794464817  ,  0.0002323300291485, -0.0004873836017367,  0.0002747145790245},
    {-0.00002578741911  ,  0.0002604647587772, -0.0004917679964223,  0.0002571372178490},
    {-0.00002146427306  ,  0.0001755130327670, -0.0002568216364782,  0.0000999559306916},
    {-0.00000271090757  , -0.0000302229676876,  0.0002153510504534, -0.0001876630582487},
    { 0.0000236114595273, -0.0002787698728746,  0.0007332437402319, -0.0004810361361027},
    { 0.0000335190097478, -0.0003361508944639,  0.0007663838741063, -0.0004510408112175},
    {-0.00002515649781  ,  0.0002843959103132, -0.0007598302896632,  0.0005576053984839}
};

static real64 DistortionBaseFunctions[16][4] = {
    {-161198.748609763,  -15217.921864762,   -1007.664204772,   15777.074701609},
    {-345482.128995846, -133268.710450586,  -33053.015941534,    6253.655659315},
    {1752896.176026130,  383286.607763863,  261790.558439266, -165147.287325005},
    {-581305.674749701, -215920.278854084,  131028.897711715,  -45924.945371228},
    {-182781.973975989, -188126.687617865,  155868.687058519,  -80284.714904594},
    {   6368.759761730, -130264.773808466,  178279.312953557,  -85267.242425694},
    { 107984.703268230,  -77125.665657832,   54794.571060910,  -29875.736897778},
    {-383297.680074417,  -74865.828707389, -150591.343668474,  105591.442584111},
    { 458885.692642048,  240804.175289493, -300590.176396289,  166821.561574621},
    {-228566.221723600,  199740.815768346, -500854.791246606,  285183.365650760},
    { -71926.941814168,  262352.319236893, -497365.509678483,  256943.950040599},
    {-220918.517572813,  147484.807006932, -266489.435114639,  114347.142650386},
    { 209640.121983343,   15400.161714264,  235896.401687523, -205950.424350574},
    {  97872.534903513, -250123.356105768,  757323.692160067, -479398.831519844},
    {  21751.116643027, -344066.168653136,  773097.812932235, -446806.375140772},
    {-105074.292183830,  258869.405554042, -778725.126824162,  559007.564481457}
};

typedef struct {const char* lens; real64 matrix[3][3];} CalibrationMatrix;
static CalibrationMatrix CalibrationLenses[2] = {
    {"FE 55mm F1.8 ZA",
     {{1.015, 0, 0     },
      {0    , 1, 0     },
      {0    , 0, 0.9707}}},
    {"FE 35mm F2.8 ZA",
     {{1.015, 0, 0     },
      {0    , 1, 0     },
      {0    , 0, 0.9604}}}
};


class ILCE7_noise_profile : public dng_noise_profile {
public:
    ILCE7_noise_profile(unsigned int iso) : dng_noise_profile(std::vector<dng_noise_function>(3)) {
        for (int i = 0; i < 28; i++)
            if (ISOlist[i] == iso) {
                for (int j = 0; j < 3; j++) {
                    fNoiseFunctions[j].SetScale(*reinterpret_cast<real64*>(&(NoiseProfileScalePerPlane[i][j])));
                    fNoiseFunctions[j].SetOffset(*reinterpret_cast<real64*>(&(NoiseProfileOffsetPerPlane[i][j])));
                }
                return;
            }
    };
};


ILCE7processor::ILCE7processor(AutoPtr<dng_host> &host, AutoPtr<dng_negative> &negative, 
                               LibRaw *rawProcessor, Exiv2::Image::AutoPtr &rawImage)
                             : NegativeProcessor(host, negative, rawProcessor, rawImage) {}


void ILCE7processor::setDNGPropertiesFromRaw() {
    NegativeProcessor::setDNGPropertiesFromRaw();

    m_negative->SetDefaultCropOrigin(dng_urational(12, 1), dng_urational(12, 1));
    m_negative->SetDefaultCropSize(dng_urational(m_RawProcessor->imgdata.sizes.width - 24, 1), 
                                   dng_urational(m_RawProcessor->imgdata.sizes.height - 24, 1));
    m_negative->SetActiveArea(dng_rect(0, 0, m_RawProcessor->imgdata.sizes.raw_height, 
                                             m_RawProcessor->imgdata.sizes.raw_width));

    m_negative->SetWhiteLevel(16300, 0);
    m_negative->SetWhiteLevel(16300, 1);
    m_negative->SetWhiteLevel(16300, 2);
    m_negative->SetWhiteLevel(16300, 3);

    dng_string makeModel;
    makeModel.Append(m_RawProcessor->imgdata.idata.make);
    makeModel.Append(" ");
    makeModel.Append(m_RawProcessor->imgdata.idata.model);
    m_negative->SetModelName(makeModel.Get());

    // -----------------------------------------------------------------------------------------
    // Read Sony's embedded CA-correction data and write WarpRectilinear opcodes

    int16 tag_CA_plane[33];
    if (getRawExifTag("Exif.SubImage1.0x7035", tag_CA_plane, 33) == 33) {
        dng_vector radialParams[3];
        dng_vector tangentialParams[3];

        for (int plane = 0; plane < 3; plane++) {
            radialParams[plane].SetIdentity(4);
            if (plane != 1) {
                int tagIndex = (plane == 2) ? 17 : 1;
                for (int param = 0; param < 4; param++) {
                    radialParams[plane][param] = 0;
                    for (int base = 0; base < 16; base++)
                        radialParams[plane][param] += (tag_CA_plane[tagIndex + base] / 128 * 
                                                       CACorrectionBaseFunctions[base][param]);
                }
                radialParams[plane][0] += 1;
            }

            tangentialParams[plane].SetIdentity(2);
            tangentialParams[plane][0] = 0;
            tangentialParams[plane][1] = 0;
        }

        radialParams[1][0] = 1;
        radialParams[1][1] = 0;
        radialParams[1][2] = 0;
        radialParams[1][3] = 0;

        dng_warp_params_rectilinear CAcorrection(3, radialParams, tangentialParams,
                                                 dng_point_real64(0.5, 0.5));
        AutoPtr<dng_opcode> opcode(new dng_opcode_WarpRectilinear(CAcorrection, 0));
        m_negative->OpcodeList3().Append(opcode);
    }

    // -----------------------------------------------------------------------------------------
    // Overwrite some fixed properties with A7-specific values

    m_negative->SetGreenSplit(250);
    m_negative->SetBaselineExposure(0.35);
    m_negative->SetBaselineNoise(0.6);
    m_negative->SetBaselineSharpness(1.33);
}


void ILCE7processor::setExifFromRaw(const dng_date_time_info &dateTimeNow, const dng_string &appNameVersion) {
    NegativeProcessor::setExifFromRaw(dateTimeNow, appNameVersion);

    dng_exif *negExif = m_negative->GetExif();

    negExif->fFocalPlaneXResolution = dng_urational(1675257385, 1000000);
    negExif->fFocalPlaneYResolution = dng_urational(1675257385, 1000000);
    negExif->fFocalPlaneResolutionUnit = ruCM;

    // Taking CFARepeatPattern out, just because Adobe doesn't write it
    negExif->fCFARepeatPatternCols = negExif->fCFARepeatPatternRows = 0;

    // -----------------------------------------------------------------------------------------
    // Write NoiseProfile depending on ISO. This is Adobe behaviour. Not sure where
    // the values come from.

    uint32 iso = negExif->fISOSpeedRatings[0];
    if (iso == 50) m_negative->SetBaselineExposure(-0.65);

    m_negative->SetNoiseProfile(ILCE7_noise_profile(iso));

    // -----------------------------------------------------------------------------------------
    // Write Calibration matrices depending on the lens. This is Adobe behaviour. Not sure where
    // the values come from.

    for (int i = 0; i < 2; i++)
        if (negExif->fLensName.Matches(CalibrationLenses[i].lens)) {
            dng_matrix matrix(3, 3);
            for (int j = 0; j < 3; j++)
                for (int k = 0; k < 3; k++)
                    matrix[j][k] = CalibrationLenses[i].matrix[j][k];
            m_negative->SetCameraCalibration1(matrix);
            m_negative->SetCameraCalibration2(matrix);
            break;
        }
    if (m_negative->ProfileCount() > 0)
        m_negative->SetCameraCalibrationSignature(m_negative->ProfileByIndex(0).ProfileCalibrationSignature().Get());
}


void ILCE7processor::setXmpFromRaw(const dng_date_time_info &dateTimeNow, const dng_string &appNameVersion) {
    NegativeProcessor::setXmpFromRaw(dateTimeNow, appNameVersion);

    dng_xmp *negXmp = m_negative->GetXMP();

    // -----------------------------------------------------------------------------------------
    // Read Sony's embedded lens distortion correction data

    int16 tag_distortion[17];
    if (getRawExifTag("Exif.SubImage1.0x7037", tag_distortion, 17) == 17) {
       real64 lensDistortInfo[4];

        for (int param = 0; param < 4; param++) {
            lensDistortInfo[param] = 0;
            for (int base = 0; base < 16; base++)
                lensDistortInfo[param] += (tag_distortion[base + 1] * DistortionBaseFunctions[base][param]);
        }
        lensDistortInfo[0] += 1073741824;
        lensDistortInfo[0] -= 32768;
        char distString[100];
        sprintf(distString, "%.0f/1073741824 %.0f/1073741824 %.0f/1073741824 %.0f/1073741824", 
                lensDistortInfo[0], lensDistortInfo[1], lensDistortInfo[2], lensDistortInfo[3]);

        negXmp->Set(XMP_NS_AUX, "LensDistortInfo", distString);
    }
}


dng_memory_stream* ILCE7processor::createDNGPrivateTag() {
    dng_memory_stream *streamPriv = NegativeProcessor::createDNGPrivateTag();

    if (!streamPriv) return NULL;

    // -----------------------------------------------------------------------------------------
    // Sony stores the offset for its SR2-IFD in the DNGPrivateData tag

    long dpdLength;
    unsigned char *dpdBuffer;
    if (!getRawExifTag("Exif.Image.DNGPrivateData", &dpdLength, &dpdBuffer)) return streamPriv;

    uint32_t SR2offset = (dpdBuffer[0] & 0xFF)       | (dpdBuffer[1] & 0xFF) << 8  | 
                         (dpdBuffer[2] & 0xFF) << 16 | (dpdBuffer[3] & 0xFF) << 24;

    // -----------------------------------------------------------------------------------------
    // Create a new 'fake' TIFF file, so we can use TiffParser to get to the SR2-tags

    unsigned char SR2IFD[114];
    const unsigned char IFDstart[8] = {0x49, 0x49, 0x2a, 0x00, 0x08, 0x00, 0x00, 0x00};
    const unsigned char IFDend[4]   = {0x00, 0x00, 0x00, 0x00};
    for (int i = 0;   i < 8;   i++) SR2IFD[i] = IFDstart[i];
    for (int i = 110; i < 113; i++) SR2IFD[i] = IFDend[i - 110];

    Exiv2::BasicIo *io = &m_RawImage->io();
    Exiv2::ExifData ee; Exiv2::IptcData ii; Exiv2::XmpData xx;
    try {
        io->open();
        io->seek(SR2offset, Exiv2::BasicIo::beg);
        io->read(&SR2IFD[8], 102);

        Exiv2::TiffParser::decode(ee, ii, xx, SR2IFD, 114);
    }
    catch (Exiv2::AnyError& e) {
        std::stringstream error; error << "Cannot open/parse proprietary Sony SR2-IFD! Exiv2 report: " << e.what();
        throw std::runtime_error(error.str());
    }

    Exiv2::ExifData::const_iterator it_offset = ee.findKey(Exiv2::ExifKey("Exif.Image.0x7200"));
    Exiv2::ExifData::const_iterator it_length = ee.findKey(Exiv2::ExifKey("Exif.Image.0x7201"));

    // -----------------------------------------------------------------------------------------
    // Write the complete SR2-IFD and SR2SubIFD into the DNGPrivateData tag.
    // This is Adobe-behaviour although it would make more sense to just write the SR2SubIFD and
    // its encryption key. Note that Adobe DNG converter (v9.0) has a bug that truncates the 
    // SR2SubIFD by 114 bytes. This version doesn't replicate the bug and writes the complete IFD

    if ((it_offset != ee.end()) && (it_length != ee.end())) {
        uint32_t SR2SubOffset = static_cast<uint32>(it_offset->toLong(0));
        uint32_t SR2SubLength = static_cast<uint32>(it_length->toLong(0));

        uint32_t fullLength = SR2SubOffset - SR2offset + SR2SubLength;
        bool padding = (fullLength & 0x01) == 0x01;

        AutoPtr<dng_memory_block> SR2block(m_host->Allocate(fullLength));
        io->seek(SR2offset, Exiv2::BasicIo::beg);
        io->read(SR2block->Buffer_uint8(), fullLength);

        streamPriv->Put("SR2 ", 4);
        streamPriv->Put_uint32(fullLength + 2 + 4);
        streamPriv->Put("II", 2 + (padding ? 1 : 0));
        streamPriv->Put_uint32(SR2offset);
        streamPriv->Put(SR2block->Buffer(), fullLength);
        if (padding) streamPriv->Put_uint8(0x00);
    }

    return streamPriv;
}

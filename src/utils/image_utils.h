#pragma once

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

#include "import_parameters.h"
#include "global.h"

struct SStbImage
{
    std::shared_ptr<unsigned char[]> m_pImage;
    UINT m_iWidth{ 0 };
    UINT m_iHeight{ 0 };
    UINT m_iChannels{ 4 };

    SStbImage();
    SStbImage(int w, int h);
    SStbImage(int w, int h, int iChannels, unsigned char* pImage);

    SStbImage(const SStbImage& image) = default;
    SStbImage(SStbImage&& image) = default;

    SStbImage& operator=(const SStbImage& image) = default;
    SStbImage& operator=(SStbImage&& image) = default;

    bool IsValid() const;
};

using TPtrImage = std::unique_ptr<SStbImage>;
using TVecImages = std::vector<SStbImage>;


TPtrImage LoadPngByStb(const std::string& filename);
bool SavePngByStb(const SStbImage& image, const std::string strFile);
bool MinimizeButton3x3(TVecImages& vecImages);
bool MinimizeButtonHorizontal(TVecImages& vecImages);
bool MinimizeButtonVertical(TVecImages& vecImages);
SStbImage SetPadding(const SStbImage& image, const int l, const int t, const int r, const int b);
TVecImages SlicePngAsReportHeader(SStbImage& image, ShadowBorder corner_radiuses);
TVecImages SlicePngAsButton3x1(SStbImage& image, int corner_radius, ShadowBorder expected_shadow_border);
TVecImages SlicePngAsButton1x3(SStbImage& image, int corner_radius, ShadowBorder expected_shadow_border);
TVecImages SlicePng3x3(SStbImage& image, int corner_radius, ShadowBorder expected_shadow_border);

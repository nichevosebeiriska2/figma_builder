#pragma once

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

#include "import_parameters.h"
#include "global.h"

struct StbImage
{
    //std::shared_ptr<unsigned char> m_pImage{ nullptr };
    std::shared_ptr<unsigned char[]> m_pImage;
    //unsigned char* m_pImage{ nullptr };
    UINT m_iWidth{ 0 };
    UINT m_iHeight{ 0 };
    UINT m_iChannels{ 4 };

    StbImage(int w, int h);
    StbImage(int w, int h, unsigned char* pImage);

    StbImage() = default;
    StbImage(const StbImage& image) = default;
    StbImage(StbImage&& image) = default;

    StbImage& operator=(const StbImage& image) = default;
    StbImage& operator=(StbImage&& image) = default;

    bool IsValid() const;
};


using TVecImages = std::vector<StbImage>;


std::optional<StbImage> LoadPngByStb(const std::string& filename);
bool SavePngByStb(const StbImage& image, const std::string strFile);

bool MinimizeButton3x3(std::vector<StbImage>& vecImages);
bool MinimizeButtonHorizontal(std::vector<StbImage>& vecImages);
bool MinimizeButtonVertical(std::vector<StbImage>& vecImages);
StbImage SetPadding(const StbImage& image, const int l, const int t, const int r, const int b);
TVecImages SlicePngAsReportHeader(StbImage& image, ShadowBorder corner_radiuses);
TVecImages SlicePngAsButton3x1(StbImage& image, int corner_radius, ShadowBorder expected_shadow_border);
TVecImages SlicePngAsButton1x3(StbImage& image, int corner_radius, ShadowBorder expected_shadow_border);
TVecImages SlicePng3x3(StbImage& image, int corner_radius, ShadowBorder expected_shadow_border);

#pragma warning(disable : 4996) // stb does use depricated functions 

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "image_utils.h"
#include "global.h"


SStbImage::SStbImage()
	: m_iWidth{ 0 }
	, m_iHeight{ 0 }
	, m_iChannels{ 0 }
	, m_pImage{ nullptr }
{

}//SStbImage::SStbImage()


SStbImage::SStbImage(int w, int h)
{
	m_iWidth = w;
	m_iHeight = h;
	m_iChannels = 4;
	m_pImage = std::make_shared<unsigned char[]>(w * h * 4);
	memset(m_pImage.get(), 0, w * h * 4);
}//StbImage::StbImage()


SStbImage::SStbImage(int w, int h, int iChannels, unsigned char* pImage)
{
	m_iWidth = w;
	m_iHeight = h;
	m_iChannels = iChannels;
	m_pImage.reset(pImage);
}//StbImage::StbImage()


bool SStbImage::IsValid() const
{
	return m_iWidth && m_iHeight && m_iChannels == 4 && m_pImage;
}//StbImage::IsValid()


TPtrImage LoadPngByStb(const std::string& filename)
{
	int iWidth = 0;
	int iHeight = 0;
	int iChannels = 0;
	unsigned char* img = stbi_load(filename.c_str(), &iWidth, &iHeight, &iChannels, 4);

	if (iWidth <= 0 || iHeight <= 0 || iChannels <= 0 || !img)
	{
		delete[] img;
		return nullptr;
	}

	return std::make_unique<SStbImage>(iWidth, iHeight, iChannels, img);
}//LoadPngByStb()


bool SavePngByStb(const SStbImage& image, const std::string strFile)
{
	return stbi_write_png(strFile.c_str(), image.m_iWidth, image.m_iHeight, image.m_iChannels, image.m_pImage.get(), image.m_iWidth * image.m_iChannels) == 1;
}//SavePngByStb()


SStbImage CopyPixels(const SStbImage& src, int x0, int y0, const int w, const int h)
{
	y0 = std::max(y0, 0);
	x0 = std::max(x0, 0);
	const int height = std::clamp<int>((h), std::min<int>(src.m_iHeight - y0, h), src.m_iHeight);
	const int width = std::clamp<int>((w), std::min<int>(src.m_iWidth - x0, w), src.m_iWidth);

	SStbImage dst(width, height);

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int srcIdx = ((y0 + y) * src.m_iWidth + (x0 + x)) * 4;
			int dstIdx = ((y)*dst.m_iWidth + (x)) * 4;

			for (int c = 0; c < 4; c++)
				dst.m_pImage[dstIdx + c] = src.m_pImage[srcIdx + c];
		}
	}

	return dst;
}//CopyPixels()


bool CopyPixels(const SStbImage& src, SStbImage& dst, const int iSrcX0, const int iSrcY0)
{
	const int iHeight = std::min<int>(dst.m_iHeight, std::max<int>((src.m_iHeight - iSrcY0),0));
	const int iWidth = std::min<int>(dst.m_iWidth, std::max<int>((src.m_iWidth - iSrcX0), 0));
	const int n_channels = 4;

	for (int y = 0; y < iHeight; y++) {
		for (int x = 0; x < iWidth; x++) {
			const int iSrcIdx = ((iSrcY0 + y) * src.m_iWidth + (iSrcX0 + x)) * n_channels;
			const int iDstIdx = ((y) * dst.m_iWidth + (x)) * n_channels;

			for (int c = 0; c < n_channels; c++)
				dst.m_pImage[iDstIdx + c] = src.m_pImage[iSrcIdx + c];
		}
	}

	return true;
}//CopyPixels()


bool CopyPixels(const SStbImage& src, SStbImage& dst, const int src_x0, const int src_y0, const int dst_x0, const int dst_y0)
{
	const int iHeight = std::min(
		std::max<int>((dst.m_iHeight - dst_y0), 0),
		std::max<int>((src.m_iHeight - src_y0), 0)
	);

	const int iWidth = std::min(
		std::max<int>((dst.m_iWidth - dst_x0), 0),
		std::max<int>((src.m_iWidth - src_x0), 0)
	);

	const int n_channels = 4;
	for (int y = 0; y < iHeight; y++) {
		for (int x = 0; x < iWidth; x++) {
			const int iSrcIdx = ((src_y0 + y) * src.m_iWidth + (src_x0 + x)) * n_channels;
			const int iDstIdx = ((dst_y0 + y)*dst.m_iWidth + (dst_x0 + x)) * n_channels;

			for (int c = 0; c < n_channels; c++) 
				dst.m_pImage[iDstIdx + c] = src.m_pImage[iSrcIdx + c];
		}
	}

	return true;
}//CopyPixels()


bool MinimizeButton3x3(TVecImages& vecImages)
{
	if (vecImages.size() != 9)
		return false;

	SStbImage& sImageLeftMiddle = vecImages[1];
	SStbImage& sImageMiddleTop = vecImages[3];
	SStbImage& sImageMiddleMiddle = vecImages[4];
	SStbImage& sImageMiddleBottom = vecImages[5];
	SStbImage& sImageRightMiddle = vecImages[7];
	
	SStbImage sImageLeftMidCopy(sImageLeftMiddle.m_iWidth, 1);
	SStbImage sImageRightMidCopy(sImageRightMiddle.m_iWidth, 1);
	SStbImage sImageMidMidCopy(1, 1);

	SStbImage sImageMidTopCopy(1, sImageMiddleTop.m_iHeight);
	SStbImage sImageMidBottomCopy(1, sImageMiddleBottom.m_iHeight);

	CopyPixels(sImageLeftMiddle, sImageLeftMidCopy, 0, 0);
	CopyPixels(sImageRightMiddle, sImageRightMidCopy, 0, 0);
	CopyPixels(sImageMiddleMiddle, sImageMidMidCopy, 0, 0);
	CopyPixels(sImageMiddleTop, sImageMidTopCopy, 0, 0);
	CopyPixels(sImageMiddleBottom, sImageMidBottomCopy, 0, 0);

	sImageLeftMiddle	= std::move(sImageLeftMidCopy);
	sImageMiddleTop		= std::move(sImageMidTopCopy);
	sImageMiddleMiddle	= std::move(sImageMidMidCopy);
	sImageMiddleBottom	= std::move(sImageMidBottomCopy);
	sImageRightMiddle	= std::move(sImageRightMidCopy);

	return true;
}//MinimizeButton3x3()


bool MinimizeButtonHorizontal(TVecImages& vecImages)
{
	if (vecImages.size() != 3)
		return false;

	SStbImage& sImageMid = vecImages[1];
	SStbImage sImageMidCopy(1, sImageMid.m_iHeight);

	CopyPixels(sImageMid, sImageMidCopy, 0, 0);

	sImageMid = std::move(sImageMidCopy);

	return true;
}//MinimizeButtonHorizontal()


bool MinimizeButtonVertical(TVecImages& vecImages)
{
	if (vecImages.size() != 3)
		return false;

	SStbImage& sImageMid = vecImages[1];
	SStbImage sImageMidCopy(sImageMid.m_iWidth, 1);

	CopyPixels(sImageMid, sImageMidCopy, 0, 0);

	sImageMid = std::move(sImageMidCopy);

	return true;
}//MinimizeButtonVertical()


SStbImage SetPadding(const SStbImage& image, const int iPaddingLeft, const int iPaddingTop, const int iPaddingRight, const int iPaddingBottom)
{
	const int iNewWidth = iPaddingLeft + iPaddingRight + image.m_iWidth;
	const int iNewHeight = iPaddingTop + iPaddingBottom + image.m_iHeight;

	SStbImage result(iNewWidth, iNewHeight);

	CopyPixels(image, result, 0,0, iPaddingLeft, iPaddingTop);

	return result;
}//SetPadding()


TVecImages SlicePngAsReportHeader(SStbImage& image, ShadowBorder corner_radiuses)
{
	auto& [fRadiusTopLeft, fRadiusTopRight, fRadiusBotLeft, fRadiusBotRight] = corner_radiuses;
	const int iLeft = std::max(std::max((int)std::ceil(fRadiusTopLeft), (int)std::ceil(fRadiusBotLeft)), 1);
	const int iRight = std::max(std::max((int)std::ceil(fRadiusTopRight), (int)std::ceil(fRadiusBotRight)), 1);
	const int iMid = image.m_iWidth - iLeft - iRight;

	return { CopyPixels(image, 0,				0,	iLeft, image.m_iHeight)
			, CopyPixels(image, iLeft,			0,	iMid, image.m_iHeight)
			, CopyPixels(image, iLeft + iMid,	0,	iRight, image.m_iHeight) };
}//SlicePngAsReportHeader()


TVecImages SlicePngAsButton3x1(SStbImage& image, int corner_radius, ShadowBorder expected_shadow_border)
{
	const int iLeft = corner_radius + expected_shadow_border.m_fLeft;
	const int iRight = corner_radius + expected_shadow_border.m_fRight;
	const int iMid = image.m_iWidth - iLeft - iRight;

	return {  CopyPixels(image, 0,				0,	iLeft, image.m_iHeight)
			, CopyPixels(image, iLeft,			0,	iMid, image.m_iHeight)
			, CopyPixels(image, iLeft + iMid,	0,	iRight, image.m_iHeight) };
}//SlicePngAsButton3x1()


TVecImages SlicePngAsButton1x3(SStbImage& image, int corner_radius, ShadowBorder expected_shadow_border)
{
	const int iTop = corner_radius + expected_shadow_border.m_fTop;
	const int iBot = corner_radius + expected_shadow_border.m_fBottom;
	const int iMid = image.m_iHeight - iTop - iBot;

	return {  CopyPixels(image, 0, 0, image.m_iWidth, iTop)
			, CopyPixels(image, 0, iTop, image.m_iWidth, iMid)
			, CopyPixels(image, 0, iTop + iMid, image.m_iWidth, iBot)};
}//SlicePngAsButton1x3()


TVecImages SlicePng3x3(SStbImage& image, int corner_radius, ShadowBorder expected_shadow_border)
{
	const int iLeftWidth = corner_radius + expected_shadow_border.m_fLeft;
	const int iRightWidth= corner_radius + expected_shadow_border.m_fRight;
	const int iMidWidth	 = image.m_iWidth - iLeftWidth - iRightWidth;

	const int iTopHeight = corner_radius + expected_shadow_border.m_fTop;
	const int iBotHeight = corner_radius + expected_shadow_border.m_fBottom;
	const int iMidHeight = image.m_iHeight - iTopHeight - iBotHeight;

	SStbImage sImageLeftTop		 = CopyPixels(image, 0, 0,						iLeftWidth, iTopHeight);
	SStbImage sImageLeftMiddle   = CopyPixels(image, 0, iTopHeight,				iLeftWidth, iMidHeight);
	SStbImage sImageLeftBottom   = CopyPixels(image, 0, iTopHeight + iMidHeight, iLeftWidth, iBotHeight);

	SStbImage sImageMiddleTop	 = CopyPixels(image, iLeftWidth, 0,						iMidWidth, iTopHeight);
	SStbImage sImageMiddleMiddle = CopyPixels(image, iLeftWidth, iTopHeight,				iMidWidth, iMidHeight);
	SStbImage sImageMiddleBottom = CopyPixels(image, iLeftWidth, iTopHeight + iMidHeight, iMidWidth, iBotHeight);

	SStbImage sImageRightTop	 = CopyPixels(image, iLeftWidth + iMidWidth, 0,						iRightWidth, iTopHeight);
	SStbImage sImageRightMiddle  = CopyPixels(image, iLeftWidth + iMidWidth, iTopHeight,				iRightWidth, iMidHeight);
	SStbImage sImageRightBottom  = CopyPixels(image, iLeftWidth + iMidWidth, iTopHeight + iMidHeight, iRightWidth, iBotHeight);

	return { std::move(sImageLeftTop),		std::move(sImageLeftMiddle),	std::move(sImageLeftBottom),
			 std::move(sImageMiddleTop),	std::move(sImageMiddleMiddle),	std::move(sImageMiddleBottom),
			 std::move(sImageRightTop),		std::move(sImageRightMiddle),	std::move(sImageRightBottom) };
}//SlicePng3x3()
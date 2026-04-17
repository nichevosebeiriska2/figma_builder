#pragma warning(disable : 4996) // stb does use depricated functions 

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "image_utils.h"
#include "global.h"


StbImage::StbImage(int w, int h)
{
	m_iWidth = w;
	m_iHeight = h;

	m_pImage = std::make_shared<unsigned char[]>(w * h * 4);
	//m_pImage2 = std::shared_ptr<unsigned char[]>(new unsigned char[w * h * 4]);
	//memset(m_pImage.get(), 0, w * h * 4);
	//m_pImage = new unsigned char[w * h * 4];
	memset(m_pImage.get(), 0, w * h * 4);
}//StbImage::StbImage()


StbImage::StbImage(int w, int h, unsigned char* pImage)
{
	m_iWidth = w;
	m_iHeight = h;

	//m_pImage2.reset(pImage);
	m_pImage.reset(pImage);
}//StbImage::StbImage()


bool StbImage::IsValid() const
{
	return m_iWidth && m_iHeight && (m_iChannels == 4) && m_pImage;
}//StbImage::IsValid()


std::optional<StbImage> LoadPngByStb(const std::string& filename)
{

	int iWidth = 0;
	int iHeight = 0;
	int iChannels = 0;
	unsigned char* img = stbi_load(filename.c_str(), &iWidth, &iHeight, &iChannels, 4);

	if (iWidth <= 0 || iHeight <= 0 || iChannels <= 0 || !img)
	{
		return std::nullopt;
		delete[] img;
	}

	return StbImage(iWidth, iHeight, img);
}//LoadPngByStb()


bool SavePngByStb(const StbImage& image, const std::string strFile)
{
	if (image.m_iChannels != 4 || image.m_iWidth <=0 || image.m_iHeight <= 0)
		int a = 1;
#pragma warning(disable : 4996)
	int result = stbi_write_png(strFile.c_str(), image.m_iWidth, image.m_iHeight, image.m_iChannels, image.m_pImage.get(), image.m_iWidth * image.m_iChannels);
	return result == 1;
}//SavePngByStb()

StbImage CopyPixels(const StbImage& src, int x0, int y0, const int w, const int h)
{
	y0 = std::max(y0, 0);
	x0 = std::max(x0, 0);
	const int height = std::clamp<int>((h), std::min<int>(src.m_iHeight - y0, h), src.m_iHeight);
	const int width = std::clamp<int>((w), std::min<int>(src.m_iWidth - x0, w), src.m_iWidth);

	StbImage dst(width, height);

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int srcIdx = ((y0 + y) * src.m_iWidth + (x0 + x)) * 4;
			int dstIdx = ((y)*dst.m_iWidth + (x)) * 4;

			for (int c = 0; c < 4; c++) {
				dst.m_pImage[dstIdx + c] = src.m_pImage[srcIdx + c];
			}
		}
	}

	return dst;
}//CopyPixels()


bool CopyPixels(const StbImage& src, StbImage& dst, const int x0, const int y0)
{
	const int height = std::min<int>(dst.m_iHeight, std::max<int>((src.m_iHeight - y0),0));
	const int width = std::min<int>(dst.m_iWidth, std::max<int>((src.m_iWidth - x0), 0));
	const int n_channels = 4;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int srcIdx = ((y0 + y) * src.m_iWidth + (x0 + x)) * n_channels;
			int dstIdx = ((y) * dst.m_iWidth + (x)) * n_channels;

			for (int c = 0; c < n_channels; c++) {
				dst.m_pImage[dstIdx + c] = src.m_pImage[srcIdx + c];
			}
		}
	}

	return true;
}//CopyPixels()


bool CopyPixels(const StbImage& src, StbImage& dst, const int src_x0, const int src_y0, const int dst_x0, const int dst_y0)
{
	const int height = std::min(
		std::max<int>((dst.m_iHeight - dst_y0), 0),
		std::max<int>((src.m_iHeight - src_y0), 0)
	);

	const int width = std::min(
		std::max<int>((dst.m_iWidth - dst_x0), 0),
		std::max<int>((src.m_iWidth - src_x0), 0)
	);

	const int n_channels = 4;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int srcIdx = ((src_y0 + y) * src.m_iWidth + (src_x0 + x)) * n_channels;
			int dstIdx = ((dst_y0 + y)*dst.m_iWidth + (dst_x0 + x)) * n_channels;

			for (int c = 0; c < n_channels; c++) {
				dst.m_pImage[dstIdx + c] = src.m_pImage[srcIdx + c];
			}
		}
	}

	return true;
}//CopyPixels()


bool MinimizeButton3x3(std::vector<StbImage>& vecImages)
{
	if (vecImages.size() != 9)
		return false;

	StbImage& lm = vecImages[1];
	StbImage& mt = vecImages[3];
	StbImage& mm = vecImages[4];
	StbImage& mb = vecImages[5];
	StbImage& rm = vecImages[7];
	
	StbImage lm_copy(lm.m_iWidth, 1);
	StbImage rm_copy(rm.m_iWidth, 1);
	StbImage mm_copy(1, 1);

	StbImage mt_copy(1, mt.m_iHeight);
	StbImage mb_copy(1, mb.m_iHeight);

	CopyPixels(lm, lm_copy, 0, 0);
	CopyPixels(rm, rm_copy, 0, 0);
	CopyPixels(mm, mm_copy, 0, 0);
	CopyPixels(mt, mt_copy, 0, 0);
	CopyPixels(mb, mb_copy, 0, 0);

	lm = std::move(lm_copy);
	mt = std::move(mt_copy);
	mm = std::move(mm_copy);
	mb = std::move(mb_copy);
	rm = std::move(rm_copy);

	return true;
}//MinimizeButton3x3()


bool MinimizeButtonHorizontal(std::vector<StbImage>& vecImages)
{
	if (vecImages.size() != 3)
		return false;

	StbImage& mid = vecImages[1];
	StbImage mid_copy(1, mid.m_iHeight);

	CopyPixels(mid, mid_copy, 0, 0);

	mid = std::move(mid_copy);

	return true;
}//MinimizeButtonHorizontal()


bool MinimizeButtonVertical(std::vector<StbImage>& vecImages)
{
	if (vecImages.size() != 3)
		return false;

	StbImage& mid = vecImages[1];
	StbImage mid_copy(mid.m_iWidth, 1);

	CopyPixels(mid, mid_copy, 0, 0);

	mid = std::move(mid_copy);

	return true;
}//MinimizeButtonVertical()


StbImage SetPadding(const StbImage& image, const int l, const int t, const int r, const int b)
{
	const int iNewWidth = l + r + image.m_iWidth;
	const int iNewHeight = t + b + image.m_iHeight;

	StbImage result(iNewWidth, iNewHeight);

	CopyPixels(image, result, 0,0,l,t);

	return result;
}//SetPadding()


TVecImages SlicePngAsReportHeader(StbImage& image, ShadowBorder corner_radiuses)
{
	auto& [r_topLeft, r_topRight, r_botLeft, r_botRight] = corner_radiuses;
	const int iLeft = std::max(std::max((int)std::ceil(r_topLeft), (int)std::ceil(r_botLeft)), 1);
	const int iRight = std::max(std::max((int)std::ceil(r_topRight), (int)std::ceil(r_botRight)), 1);
	const int iMid = image.m_iWidth - iLeft - iRight;

	return { CopyPixels(image, 0,				0,	iLeft, image.m_iHeight)
			, CopyPixels(image, iLeft,			0,	iMid, image.m_iHeight)
			, CopyPixels(image, iLeft + iMid,	0,	iRight, image.m_iHeight) };
}//SlicePngAsReportHeader()


TVecImages SlicePngAsButton3x1(StbImage& image, int corner_radius, ShadowBorder expected_shadow_border)
{
	const int iLeft = corner_radius + expected_shadow_border.m_fLeft;
	const int iRight = corner_radius + expected_shadow_border.m_fRight;
	const int iMid = image.m_iWidth - iLeft - iRight;

	return {  CopyPixels(image, 0,				0,	iLeft, image.m_iHeight)
			, CopyPixels(image, iLeft,			0,	iMid, image.m_iHeight)
			, CopyPixels(image, iLeft + iMid,	0,	iRight, image.m_iHeight) };
}//SlicePngAsButton3x1()


TVecImages SlicePngAsButton1x3(StbImage& image, int corner_radius, ShadowBorder expected_shadow_border)
{
	const int iTop = corner_radius + expected_shadow_border.m_fTop;
	const int iBot = corner_radius + expected_shadow_border.m_fBottom;
	const int iMid = image.m_iHeight - iTop - iBot;

	return {  CopyPixels(image, 0, 0, image.m_iWidth, iTop)
			, CopyPixels(image, 0, iTop, image.m_iWidth, iMid)
			, CopyPixels(image, 0, iTop + iMid, image.m_iWidth, iBot)};
}//SlicePngAsButton1x3()


TVecImages SlicePng3x3(StbImage& image, int corner_radius, ShadowBorder expected_shadow_border)
{
	const int iLeftWidth = corner_radius + expected_shadow_border.m_fLeft;
	const int iRightWidth= corner_radius + expected_shadow_border.m_fRight;
	const int iMidWidth	 = image.m_iWidth - iLeftWidth - iRightWidth;

	const int iTopHeight = corner_radius + expected_shadow_border.m_fTop;
	const int iBotHeight = corner_radius + expected_shadow_border.m_fBottom;
	const int iMidHeight = image.m_iHeight - iTopHeight - iBotHeight;

	StbImage lt = CopyPixels(image, 0, 0,						iLeftWidth, iTopHeight);
	StbImage lm = CopyPixels(image, 0, iTopHeight,				iLeftWidth, iMidHeight);
	StbImage lb = CopyPixels(image, 0, iTopHeight + iMidHeight, iLeftWidth, iBotHeight);

	StbImage mt = CopyPixels(image, iLeftWidth, 0,						iMidWidth, iTopHeight);
	StbImage mm = CopyPixels(image, iLeftWidth, iTopHeight,				iMidWidth, iMidHeight);
	StbImage mb = CopyPixels(image, iLeftWidth, iTopHeight + iMidHeight, iMidWidth, iBotHeight);

	StbImage rt = CopyPixels(image, iLeftWidth + iMidWidth, 0,						iRightWidth, iTopHeight);
	StbImage rm = CopyPixels(image, iLeftWidth + iMidWidth, iTopHeight,				iRightWidth, iMidHeight);
	StbImage rb = CopyPixels(image, iLeftWidth + iMidWidth, iTopHeight + iMidHeight, iRightWidth, iBotHeight);

	return { std::move(lt), std::move(lm), std::move(lb),
			 std::move(mt), std::move(mm), std::move(mb),
			 std::move(rt), std::move(rm), std::move(rb) };
}//SlicePng3x3()

#include <algorithm>
#include <ranges>

#include "import.h"
#include "global.h"
#include "ApiWrapper.h"


ShadowBorder CImporter::GetReportHeaderSizes(rj::Value& element_json)
{
	if (!element_json.HasMember("rectangleCornerRadii") || !element_json["rectangleCornerRadii"].IsArray())
		return {};

	const rj::Value& corner_radiuses = element_json["rectangleCornerRadii"].GetArray();

	return {  corner_radiuses[0].GetFloat()
			, corner_radiuses[1].GetFloat()
			, corner_radiuses[2].GetFloat()
			, corner_radiuses[3].GetFloat() };
}


StbImage CImporter::StretchToExpectedSize(rj::Value& element_json, StbImage& image, ShadowBorder sBorder, float scale)
{
	const ShadowBorder sBorderOwn = sBorder * scale;
	const ShadowBorder sBorderEffects = CalcShadowBorderForElement(element_json) * scale;
	ShadowBorder sBorderSpace;
	if (sBorderOwn < sBorderEffects)
	{
		Logger::Error(std::format("For element [{}] specified border is less than border of effects.\n Border switcher to effects border", element_json["name"].GetString()), 1);
		ShadowBorder sBorderSpace = sBorderEffects;
	}
	else
		sBorderSpace = sBorderOwn - sBorderEffects;

	return SetPadding(image, sBorderSpace.m_fLeft, sBorderSpace.m_fTop, sBorderSpace.m_fRight, sBorderSpace.m_fBottom);
}


bool CheckElementHasGradient(rj::Value& element_json)
{
	if (!element_json.HasMember("fills"))
		return false;

	const rj::Value& arrFills = element_json["fills"].GetArray();

	for (auto it = arrFills.Begin(); it != arrFills.End(); it++)
		if (it->HasMember("type") && std::string{ (*it)["type"].GetString() } != std::string{ "SOLID" } && it->HasMember("visible") && (*it)["visible"].GetBool())
		{
			//auto type = std::string{ (*it)["type"].GetString() };
			//auto visible = (*it)["visible"].GetBool();
			return true;
		}

	return false;
}


bool SaveImagesVectorWithSuffixes(const TVecImages& vecImages, const std::string& strElementName, float scale, const auto& arrSuffixes)
{
	if (vecImages.size() != arrSuffixes.size())
		return false;

	for (auto [pos, suffix] : std::views::enumerate(arrSuffixes))
		if (!SavePngByStb(vecImages[pos++], CreateOutputImagePath(strElementName, suffix, scale).generic_string()))
			return false;

	return true;
}

bool SaveButtonHorizontal(const TVecImages& vecImages,const std::string& strElementName, float scale)
{
	return SaveImagesVectorWithSuffixes(vecImages, strElementName, scale, std::array{ "_left","_middle","_right" });
}

bool SaveButtonVertical(const TVecImages& vecImages, const std::string& strElementName, float scale)
{
	return SaveImagesVectorWithSuffixes(vecImages, strElementName, scale, std::array{ "_top","_middle","_bottom" });
}

bool SaveButton3x3(const TVecImages& vecImages, const std::string& strElementName, float scale)
{
	return SaveImagesVectorWithSuffixes(vecImages, strElementName, scale
		, std::array{
			"_topleft","_middleleft","_bottomleft",
			"_topcenter","_middlecenter","_bottomcenter",
			"_topright","_middleright","_bottomright",
		});
}

bool SaveButton(ESliceFormat eFormat, const TVecImages& vecImages, const std::string& strElementName, float scale)
{
	switch (eFormat)
	{
		case(ESliceFormat_ButtonVertical):
			return SaveButtonVertical(vecImages, strElementName, scale);

		case(ESliceFormat_ButtonHorizontal):
			return SaveButtonHorizontal(vecImages, strElementName, scale);

		case(ESliceFormat_Button3x3):
			return SaveButton3x3(vecImages, strElementName, scale);
	}

	return false;
}


bool CImporter::Import(const std::string strElementName, rj::Value& elemJson, StbImage& image, ESliceFormat eFormat, ShadowBorder sBorder, float scale)
{
	if (eFormat == ESliceFormat_Ignore)
		return false;

	const auto border = sBorder * scale;
	auto image_of_required_size = StretchToExpectedSize(elemJson, image, sBorder, scale);

	if (image.m_iHeight <= 0 || image.m_iWidth <= 0)
		return false;

	switch (eFormat) {
		case(ESliceFormat_Single): {
			return SavePngByStb(image_of_required_size, CreateOutputImagePath(strElementName, "", scale).generic_string());
		}
		case(ESliceFormat_ButtonHorizontal): {

			if (strElementName.find("button_conte") != strElementName.npos)
				int a = 1;
			float corner_radius = elemJson.HasMember("cornerRadius") ? elemJson["cornerRadius"].GetFloat() * scale : 1.0f;
			corner_radius = std::min<int>(corner_radius, std::min(0.5 * image.m_iWidth, 0.5 * image.m_iHeight));

			if (std::abs(corner_radius) < 0.01)//rect of height == 1 (separator for example)
				corner_radius = 1.0f;

			if (corner_radius * 2 >= image.m_iWidth ) // rect degenerater to circle
				return Import(strElementName, elemJson, image, ESliceFormat_Single, sBorder, scale);

			auto vecImages = SlicePngAsButton3x1(image_of_required_size, corner_radius, border);

			if (!CheckElementHasGradient(elemJson))
				MinimizeButtonHorizontal(vecImages);

			return SaveButton(ESliceFormat_ButtonHorizontal, vecImages, strElementName, scale);

			break;
		}
		case(ESliceFormat_ButtonVertical): {
			float corner_radius = elemJson.HasMember("cornerRadius") ? elemJson["cornerRadius"].GetFloat() * scale : 1.0f;
			corner_radius = std::min<int>(corner_radius, std::min(0.5 * image.m_iWidth, 0.5 * image.m_iHeight));
			
			if (std::abs(corner_radius) < 0.01)//rect of height == 1 (separator for example)
				corner_radius = 1.0f;

			if (corner_radius * 2 >= image.m_iHeight) // rect degenerater to circle
				return Import(strElementName, elemJson, image, ESliceFormat_Single, sBorder, scale);

			TVecImages vecImages = SlicePngAsButton1x3(image_of_required_size, corner_radius, border);

			if (!CheckElementHasGradient(elemJson))
				MinimizeButtonVertical(vecImages);

			return SaveButton(ESliceFormat_ButtonVertical, vecImages, strElementName, scale);
		}
		case(ESliceFormat_ReportGrid): [[fallthrough]];
		case(ESliceFormat_ButtonAuto): [[fallthrough]];
		case(ESliceFormat_Button3x3): {

			const int iWidth = image.m_iWidth;
			const int iHeight = image.m_iHeight;

			// if element has no corner radius make it eq to 1
			const float corner_radius = elemJson.HasMember("cornerRadius") ? elemJson["cornerRadius"].GetFloat() * scale : 1.0f;

			// corner radius can be any value; figma adjust corner radius to std::min(radius, height*0.5)
			// if corner radius > 0.5*width you should import image as horizontal button instead
			if (corner_radius * 2 >= iHeight)
				return Import(strElementName, elemJson, image, ESliceFormat_ButtonHorizontal, sBorder, scale);

			if (corner_radius * 2 >= iWidth)
				return Import(strElementName, elemJson, image, ESliceFormat_ButtonVertical, sBorder, scale);

			TVecImages vecImages = SlicePng3x3(image_of_required_size, corner_radius, border);

			if (!CheckElementHasGradient(elemJson))
				MinimizeButton3x3(vecImages);

			return SaveButton(ESliceFormat_Button3x3, vecImages, strElementName, scale);
		}
		case(ESliceFormat_ReportHeader): {
			auto vecImages = SlicePngAsReportHeader(image, GetReportHeaderSizes(elemJson) * scale);
			MinimizeButtonHorizontal(vecImages);

			return SaveButton(ESliceFormat_ButtonHorizontal, vecImages, strElementName, scale);
		}
	}


	return false;
}
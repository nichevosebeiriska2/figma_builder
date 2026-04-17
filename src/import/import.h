#pragma once

#include <optional>

#include "image_utils.h"
#include "import_parameters.h"
#include "json_utils.h"

class CImporter
{
protected:
	static ShadowBorder GetReportHeaderSizes(rj::Value& element_json);
	static StbImage StretchToExpectedSize(rj::Value& element_json, StbImage& image, ShadowBorder sBorder, float scale);

public:
	static bool Import(const std::string strElementName, rj::Value& elemJson, StbImage& image, ESliceFormat eFormat, ShadowBorder sBorder, float scale);
};
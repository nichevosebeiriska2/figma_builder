#pragma once

#include <optional>

#include "image_utils.h"
#include "import_parameters.h"
#include "json_utils.h"

class CImporter
{
protected:
	static ShadowBorder GetReportHeaderSizes(rj::Value& element_json);
	static SStbImage StretchToExpectedSize(rj::Value& element_json, SStbImage& image, ShadowBorder sBorder, float scale);

public:
	static bool Import(const std::string strElementName, rj::Value& elemJson, SStbImage& image, ESliceFormat eFormat, ShadowBorder sBorder, float scale);
	//static rj::Document PrecomputeBorder(rj::Value& elementJson);
};
#include "json_utils.h"

#include "writer.h"
#include "prettywriter.h"
#include "filewritestream.h"
#include "ostreamwrapper.h"

#include <filereadstream.h>
#include <iostream>
#include <fstream>

using namespace rj;


void SaveJsonToFileFast(const rj::Value& doc, std::filesystem::path path)
{
	using namespace rj;

	if (!std::filesystem::exists(path.parent_path()))
		std::filesystem::create_directories(path.parent_path());

	FILE* fp = NULL;
	fopen_s(&fp, path.generic_string().c_str(), "wb");
	char writeBuffer[64* 1024]; // 2 МБ
	rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));

	rapidjson::Writer<rapidjson::FileWriteStream> writer(os);

	doc.Accept(writer);


	writer.Flush();
	fclose(fp);
}


std::string JsonToString(const rj::Value& value)
{
	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	value.Accept(writer);
	
	return buffer.GetString();
}


void SaveJsonToFile(const rj::Value& doc, std::filesystem::path path)
{
	using namespace rj;

	if (!std::filesystem::exists(path.parent_path()))
		std::filesystem::create_directories(path.parent_path());

	std::ofstream ofs(path);
	rapidjson::OStreamWrapper osw(ofs);
	rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
	doc.Accept(writer);
}


rj::Document LoadJsonFromFileFast(std::filesystem::path path)
{
	if (std::filesystem::exists(path))
	{
		rj::Document doc(kObjectType);
		std::string strContent;
		FILE* file;
		fopen_s(&file, path.generic_string().c_str(), "rb");
		char buf[8 * 1024];
		rj::FileReadStream frs(file, buf, sizeof(buf));
		doc.ParseStream(frs);
		return doc;
		fclose(file);
	}

	return rj::Document(kNullType);
}


rj::Document LoadJsonFromFile(std::filesystem::path path)
{
	std::ifstream file(path, std::ios::binary);
	if (file.is_open())
	{
		rj::Document doc(kObjectType);
		std::string strContent;
		strContent.reserve(std::filesystem::file_size(path));
		strContent = { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
		doc.Parse<rj::kParseCommentsFlag>(strContent.c_str());
		
		return doc;
	}

	return rj::Document(kNullType);
}


bool ParseJsonArrayAsFloats(const rj::Value& valArray, TVecFloat& vecResult)
{
	if (!valArray.IsArray())
		return false;

	for (auto it = valArray.Begin(); it != valArray.End(); it++)
	{
		if (!it->IsFloat())
			return false;

		vecResult.emplace_back(it->GetFloat());
	}

	return true;
}


bool ParseJsonArrayAsInts(const rj::Value& valArray, TVecInts& vecResult)
{
	if (!valArray.IsArray())
		return false;

	for (auto it = valArray.Begin(); it != valArray.End(); it++)
	{
		if (!it->IsInt())
			return false;

		vecResult.emplace_back(it->GetInt());
	}

	return true;
}



bool ParseJsonArrayAsStrings(const rj::Value& valArray, TVecStr& vecResult)
{
	if (!valArray.IsArray())
		return false;

	for (auto it = valArray.Begin(); it != valArray.End(); it++)
	{
		if (!it->IsString())
			return false;

		vecResult.emplace_back(it->GetString());
	}

	return true;
}


void FindNodesByNameAndType(const Value& node, const std::string& key, const std::string& value, TVecValuePtrs& results) {

	if (node.IsObject()) {
		if (node.HasMember(key.c_str()) && node[key.c_str()].IsString())
			if (std::string{ node[key.c_str()].GetString() } == value)
				results.push_back(&const_cast<Value&>(node[key.c_str()]));

		for (auto it = node.MemberBegin(); it != node.MemberEnd(); it++)
			FindNodesByNameAndType(it->value, key, value, results);

	}
	else if (node.IsArray())
		for (auto it = node.Begin(); it != node.End(); it++)
			FindNodesByNameAndType(*it, key, value, results);

}


void FindNodesByNameAndType(const rj::Value& valueNode, const std::string& name, ENodeType eType, TVecValuePtrs& results)
{
	if (valueNode.IsObject()) {
		if (valueNode.HasMember("name") && valueNode.HasMember("type") && valueNode["name"].GetString() == name && valueNode["type"].GetString() == ToString(eType))
			results.push_back(&const_cast<Value&>(valueNode));

		for (auto it = valueNode.MemberBegin(); it != valueNode.MemberEnd(); it++)
			FindNodesByNameAndType(it->value, name, eType, results);

	}
	else if (valueNode.IsArray())
		for (auto it = valueNode.Begin(); it != valueNode.End(); it++)
			FindNodesByNameAndType(*it, name, eType, results);
}


void FindNodesByNameAndType(const rj::Value& valueNode, const TVecStr& vecStrNames, ENodeType eType, TVecValuePtrs& results)
{
	if (valueNode.IsObject()) {
	{
		bool bNodeFinded = valueNode.HasMember("name") 
			&& valueNode.HasMember("type")
			&& valueNode["type"].GetString() == ToString(eType)
			&& std::ranges::find(vecStrNames, valueNode["name"].GetString()) != vecStrNames.end();
		if (bNodeFinded)
			results.push_back(&const_cast<Value&>(valueNode));
	}


		for (auto it = valueNode.MemberBegin(); it != valueNode.MemberEnd(); it++)
			FindNodesByNameAndType(it->value, vecStrNames, eType, results);

	}
	else if (valueNode.IsArray())
		for (auto it = valueNode.Begin(); it != valueNode.End(); it++)
			FindNodesByNameAndType(*it, vecStrNames, eType, results);
}

void findNodesByType(const TValue& valueNode, ENodeType eType, TVecValuePtrs& results)
{
	if (valueNode.IsObject()) {
 		if (valueNode.HasMember("type") && StringToNodeType(valueNode["type"].GetString()) == eType)
			results.push_back(&valueNode);


		for (auto it = valueNode.MemberBegin(); it != valueNode.MemberEnd(); it++)
			findNodesByType(it->value, eType, results);

	}
	else if (valueNode.IsArray())
		for (auto it = valueNode.Begin(); it != valueNode.End(); it++)
			findNodesByType(*it, eType, results);
}


bool IsNodeGraphicalElement(const rj::Value& valueNode)
{
	if (!valueNode.HasMember("type") || (valueNode.HasMember("visible") && valueNode["visible"].GetBool() == false))
		return false;

	const ENodeType eType = StringToNodeType(valueNode["type"].GetString());

	return eType == RECTANGLE || eType == VECTOR || eType == ELLIPSE || eType == BOOLEAN_OPERATION || eType == SLICE || eType == FRAME;
}


void FindElementsFromComponents(const rj::Value& node_export_dev, TVecValuePtrs& results)
{
	const TValue& children = node_export_dev["children"].GetArray();

	for (auto it = children.Begin(); it != children.End(); it++)
	{
		const TValue& valueNode = *it;

		if (!valueNode.IsObject())
			continue;

		if (IsNodeGraphicalElement(valueNode)) // collect each visible element with allowed type
			results.emplace_back(&valueNode);

		// if non-instance element has children and visible find elements within
		if (valueNode.HasMember("children") && StringToNodeType(valueNode["type"].GetString()) != INSTANCE && (!valueNode.HasMember("visible") || !valueNode["visible"].GetBool()))
			FindElementsFromComponents(valueNode, results);
	}
}
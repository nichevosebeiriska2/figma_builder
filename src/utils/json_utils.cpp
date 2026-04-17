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


std::optional<TVecFloat> ParseJsonArrayAsFloats(const rj::Value& valArray)
{
	if (!valArray.IsArray())
		return std::nullopt;

	TVecFloat result;

	for (auto it = valArray.Begin(); it != valArray.End(); it++)
	{
		if (!it->IsFloat())
			return std::nullopt;

		result.emplace_back(it->GetFloat());
	}

	return result;
}


std::optional<TVecInts> ParseJsonArrayAsInts(const rj::Value& valArray)
{
	if (!valArray.IsArray())
		return std::nullopt;

	TVecInts result;

	for (auto it = valArray.Begin(); it != valArray.End(); it++)
	{
		if (!it->IsInt())
			return std::nullopt;

		result.emplace_back(it->GetInt());
	}

	return result;
}



std::optional<TVecStr> ParseJsonArrayAsStrings(const rj::Value& valArray)
{
	if (!valArray.IsArray())
		return std::nullopt;

	TVecStr result;

	for (auto it = valArray.Begin(); it != valArray.End(); it++)
	{
		if (!it->IsString())
			return std::nullopt;

		result.emplace_back(it->GetString());
	}

	return result;
}


void findNodesByNameAndType(const Value& node, const std::string& key, const std::string& value, std::vector<Value*>& results) {

	if (node.IsObject()) {
		if (node.HasMember(key.c_str()) && node[key.c_str()].IsString())
			if (std::string{ node[key.c_str()].GetString() } == value)
				results.push_back(&const_cast<Value&>(node[key.c_str()]));

		for (auto it = node.MemberBegin(); it != node.MemberEnd(); it++)
			findNodesByNameAndType(it->value, key, value, results);

	}
	else if (node.IsArray())
		for (auto it = node.Begin(); it != node.End(); it++)
			findNodesByNameAndType(*it, key, value, results);

}


void findNodesByNameAndType(const rj::Value& node, const std::string& name, ENodeType eType, std::vector<rj::Value*>& results)
{
	if (node.IsObject()) {
		if (node.HasMember("name") && node["name"].IsString() && node.HasMember("type") && node["type"].IsString())
		{
			if (std::string{ node["name"].GetString() } == name && std::string{ node["type"].GetString() } == ToString(eType))
				results.push_back(&const_cast<Value&>(node));

		}
			

		for (auto it = node.MemberBegin(); it != node.MemberEnd(); it++)
			findNodesByNameAndType(it->value, name, eType, results);

	}
	else if (node.IsArray())
		for (auto it = node.Begin(); it != node.End(); it++)
			findNodesByNameAndType(*it, name, eType, results);
}


bool IsNodeGraphicalElement(const rj::Value& node)
{
	if (!node.HasMember("type") || (node.HasMember("visible") && node["visible"].GetBool() == false))
		return false;

	const std::string strType = node["type"].GetString();

	return (strType == "RECTANGLE" || strType == "VECTOR" || strType == "ELLIPSE" || strType == "BOOLEAN_OPERATION" || strType == "SLICE" || strType == "FRAME");
}


void findElementNodesFromExportDev(const rj::Value& node_export_dev, std::vector<const rj::Value*>& results)
{
	auto children = node_export_dev["children"].GetArray();

	for (auto it = children.Begin(); it != children.End(); it++)
	{
		if (!it->IsObject())
			continue;

		if (IsNodeGraphicalElement(*it))
			results.emplace_back(&(*it));

		if (it->HasMember("children") && std::string{ (*it)["type"].GetString() } != std::string{ "INSTANCE" })
			findElementNodesFromExportDev(*it, results);
	}
}
#pragma once

#include <vector>
#include <string>
#include <map>
#include <filesystem>
#include "rapidjson.h"
#include "document.h"

#include "global.h"

namespace rj = rapidjson;

// with pretty write
void SaveJsonToFile(const rj::Value& doc, std::filesystem::path path);
rj::Document LoadJsonFromFile(std::filesystem::path path); 
// no pretty write!
rj::Document LoadJsonFromFileFast(std::filesystem::path path);
void SaveJsonToFileFast(const rj::Value& doc, std::filesystem::path path);

std::optional<TVecFloat> ParseJsonArrayAsFloats(const rj::Value& valArray);
std::optional<TVecInts>	 ParseJsonArrayAsInts(const rj::Value& valArray);
std::optional<TVecStr>   ParseJsonArrayAsStrings(const rj::Value& valArray);

void findNodesByNameAndType(const rj::Value& node, const std::string& key, const std::string& value, std::vector<rj::Value*>& results);
void findNodesByNameAndType(const rj::Value& node, const std::string& name, ENodeType eType, std::vector<rj::Value*>& results);

void findElementNodesFromExportDev(const rj::Value& node_export_dev, std::vector<const rj::Value*>& results);
bool IsNodeGraphicalElement(const rj::Value& node);
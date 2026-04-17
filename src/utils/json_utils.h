#pragma once

#include <vector>
#include <string>
#include <map>
#include <filesystem>
#include "rapidjson.h"
#include "document.h"

#include "global.h"

namespace rj = rapidjson;
using TValue = rj::Value;
using TDocument = rj::Document;
using TVecValuePtrs = std::vector<const TValue*>;

// with pretty write
void SaveJsonToFile(const rj::Value& doc, std::filesystem::path path);
rj::Document LoadJsonFromFile(std::filesystem::path path); 
// no pretty write! (optimised for large files)
rj::Document LoadJsonFromFileFast(std::filesystem::path path);
void SaveJsonToFileFast(const rj::Value& doc, std::filesystem::path path);

std::string JsonToString(const rj::Value& value);
bool ParseJsonArrayAsFloats(const rj::Value& valArray, TVecFloat& vecResult);
bool ParseJsonArrayAsInts(const rj::Value& valArray, TVecInts& vecResult);
bool ParseJsonArrayAsStrings(const rj::Value& valArray, TVecStr& vecResult);

void FindNodesByNameAndType(const rj::Value& node, const std::string& key, const std::string& value, TVecValuePtrs& results);
void FindNodesByNameAndType(const rj::Value& node, const std::string& name, ENodeType eType, TVecValuePtrs& results);
void FindNodesByNameAndType(const rj::Value& node, const TVecStr& vecStrNames, ENodeType eType, TVecValuePtrs& results);
void findNodesByType(const TValue& node, ENodeType eType, TVecValuePtrs& results);

void FindElementsFromComponents(const rj::Value& node_export_dev, TVecValuePtrs& results);
bool IsNodeGraphicalElement(const rj::Value& node);
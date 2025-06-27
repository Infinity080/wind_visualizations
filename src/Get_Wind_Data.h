#include <string>
#include <vector>
#include "json.hpp"
#include <ghc/filesystem.hpp>
#include <cpr/cpr.h>
#include <fstream>
#include <chrono>
#include <sstream>

namespace fs = ghc::filesystem;
using json = nlohmann::json;

extern std::vector<std::string> global_latitudes;
extern std::vector<std::string> global_longitudes;

std::string GetFormattedDate(int offset_days);

std::string GetWindDataGlobal(std::string date);
int FetchWindDataGlobal();
int ConvertGribToJson(const std::string& gribFile,
    const std::string& jsonFile);
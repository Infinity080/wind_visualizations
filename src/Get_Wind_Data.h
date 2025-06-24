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

std::string GetWindDataGlobal();
void CacheWindDataGlobal(const std::string& data);
std::string FetchWindDataGlobal();

std::string GetWindData(const std::vector<std::string>& latitudes, const std::vector<std::string>& longitudes);

/*
void CacheWindData(const std::string& data, const std::vector<std::string>& latitudes, const std::vector<std::string>& longitudes);
std::string FetchWindData(const std::vector<std::string>& latitudes, const std::vector<std::string>& longitudes);
*/
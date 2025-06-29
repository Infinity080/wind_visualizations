#include "Get_Wind_Data.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
std::vector<std::string> global_latitudes;
std::vector<std::string> global_longitudes;

int days_before = 2; // Maks. 7 dni (zalecane)

bool OFFLINE_MODE = false;

// Funkcja do formatowania daty pocz¹tkowej i koñcowej do requesta
std::string GetFormattedDate(int offset_days) {
    auto now = std::chrono::system_clock::now();
    auto target_time = now + std::chrono::hours(24 * offset_days);
    std::time_t time = std::chrono::system_clock::to_time_t(target_time);
    std::tm tm = *std::localtime(&time);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d");
    return oss.str();
}


// Funkcja pomocnicza (Dzieli liniê wed³ug delimitera)
static std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// // Funkcja pomocnicza (Usuwa otaczaj¹ce cudzys³owy i bia³e znaki)
static std::string trimQuotes(const std::string& s) {
    size_t start = 0, end = s.size();
    while (start < end && std::isspace((unsigned char)s[start])) ++start;
    while (end > start && std::isspace((unsigned char)s[end - 1])) --end;
    if (end - start >= 2 && s[start] == '"' && s[end - 1] == '"') {
        ++start;
        --end;
    }
    return s.substr(start, end - start);
}

// Funkcja do konwersji pliku GRIB do pliku JSON
int ConvertGribToJson(const std::string& gribFile,
    const std::string& jsonFile) {
    const std::string csvFile = "temp_wind_data.csv";

    // Konwersja pliku GRIB do CSV za pomoc¹ wgrib2
    std::string cmd = "wgrib2.exe " + gribFile + " -csv " + csvFile;
    if (std::system(cmd.c_str()) != 0) {
        std::cerr << "B³¹d podczas wywo³ania wgrib2" << std::endl;
        return 1;
    }

    std::ifstream in(csvFile);
    if (!in.is_open()) {
        std::cerr << "Nie uda³o siê otworzyæ pliku CSV: " << csvFile << std::endl;
        return 2;
    }

    // Pomijamy pierwszy wiersz (nag³ówek wgrib2)
    std::string headerLine;
    std::getline(in, headerLine);

	// Definiujemy klucze dla pliku JSON
    std::vector<std::string> headers = {
        "start_date",
        "forecast_date",
        "parameter",
        "level",
        "longitude",
        "latitude",
        "value"
    };

    json result = json::array();
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto rawFields = split(line, ',');

		// Jeœli liczba pól nie zgadza siê z nag³ówkami, pomijamy liniê
        if (rawFields.size() != headers.size()) continue;

        json obj;
        for (size_t i = 0; i < headers.size(); ++i) {
            std::string value = trimQuotes(rawFields[i]);
            const auto& key = headers[i];

			// Zamiana wartoœci liczbowych na odpowiednie typy
            if (key == "longitude" || key == "latitude" || key == "value") {
                try {
                    obj[key] = std::stod(value);
                }
                catch (const std::invalid_argument&) {
                    obj[key] = value; // Zapisz jako string w razie b³êdu
                }
            }
            else {
                obj[key] = value;
            }
        }
        result.push_back(obj);
    }
    in.close();

    // Zapis pliku JSON
    std::ofstream out(jsonFile);
    if (!out.is_open()) {
        std::cerr << "Nie mo¿na otworzyæ pliku do zapisu: " << jsonFile << std::endl;
        return 3;
    }
    out << result.dump(2);
    out.close();

    std::remove(csvFile.c_str());
    return 0;
}

// Funkcja do pobierania danych z backupowego pliku JSON
std::string GetBackupData() {
    const char* backupFile = "backup_wind_data.json";
    std::ifstream backup(backupFile);

    // Jeœli plik backupowy istnieje, wczytaj dane
    if (backup.is_open()) {
        nlohmann::json backup_data;
        backup >> backup_data;
        return backup_data.dump();
    }
    else {
        throw std::runtime_error("Nie udalo sie odczytac danych z pliku backup");
    }
}

// Funkcja do ³adowania danych o wietrze z cache lub API dla globalnych wspó³rzêdnych
std::string GetWindDataGlobal(std::string date) {
    if (OFFLINE_MODE) {
        std::cout << "Tryb offline - korzystanie z pliku backupowego" << std::endl;
        return GetBackupData();
	}
    std::string cacheFile = date + "_wind_data.json";

    // Jeœli plik cache nie istnieje, pobierz dane z API
    if (!fs::exists(cacheFile)) {
        if (FetchWindDataGlobal() != 0) {
            return GetBackupData();
		}
    }

    // Otwórz plik cache
    std::ifstream cache(cacheFile);

    // Jeœli nie uda³o siê otworzyæ pliku cache, pobierz dane z API
    if (!cache.is_open()) {
        if (FetchWindDataGlobal() != 0) {
			return GetBackupData();
		}
        std::ifstream cache(cacheFile);
    }

    // Wczytanie danych z pliku cache
    std::stringstream buffer;
    buffer << cache.rdbuf();
    return buffer.str();
}


// Funkcja do pobierania danych o wietrze dla globalnych wspó³rzêdnych i zapisywania ich do pliku JSON
int FetchWindDataGlobal() {
    const std::string cycle = "00";           // cykl 0 UTC
    const std::string file = "gfs.t" + cycle + "z.pgrb2.1p00.f000";


    // Tworzymy requesty do API dla ka¿dego dnia
    for (int day = 0; day <= days_before; ++day) {
        std::string date = GetFormattedDate(-day);

        if (fs::exists(date + "_wind_data.json")) {
            continue;
        }

        const std::string dir = "%2Fgfs." + date + "%2F" + cycle + "%2Fatmos";
        const std::string data_url = "https://nomads.ncep.noaa.gov/cgi-bin/filter_gfs_1p00.pl?"
            "file=" + file +
            "&var_GUST=on&var_UGRD=on&var_VGRD=on"      // Zmienne
            "&lev_10_m_above_ground=on&lev_surface=on"  // Poziomy
            "&dir=" + dir;                              // Katalog (na koñcu)

        std::cout << data_url << std::endl;
        // Wysy³amy request do API
        auto response = cpr::Get(
            cpr::Url{ data_url }
        );

        if (response.status_code == 200) {
            // Jeœli request siê powiód³, zapisujemy dane do pliku GRIB
            std::string gribFile = date + "_wind_data.grib";
            std::ofstream outFile(gribFile, std::ios::binary);
            outFile.write(response.text.c_str(), response.text.size());
            outFile.close();

            // Konwertujemy plik GRIB do JSON
            std::string jsonFile = date + "_wind_data.json";
            if (ConvertGribToJson(gribFile, jsonFile) != 0) {
                std::cout << "B³¹d podczas konwersji pliku GRIB do JSON dla daty: " << date << std::endl;
                return -1;
            }

        }
        else
            if (response.status_code != 200) {
                std::cout << "Wystapil blad " << response.status_code << " podczas pobierania danych z API. Wykorzystany zostanie backupowy plik JSON" << std::endl;
                return -1;
            }
    }
    return 0;
}
#include "Get_Wind_Data.h"
#include <iostream>

std::vector<std::string> global_latitudes;
std::vector<std::string> global_longitudes;

int days_before = 1; // Maks. 7 dni (zalecane)
int days_after = 1; // Maks 90 dni

// Funkcja do formatowania daty pocz¹tkowej i koñcowej do requesta
std::string GetFormattedDate(int offset_days) {
    auto now = std::chrono::system_clock::now();
    auto target_time = now + std::chrono::hours(24 * offset_days);
    std::time_t time = std::chrono::system_clock::to_time_t(target_time);
    std::tm tm = *std::localtime(&time);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

// Funkcja do cachowania danych o wietrze z API dla globalnych wspó³rzêdnych
void CacheWindDataGlobal(const std::string& data) {
    const char* cacheFile = "global_wind_data_cache.json";
    std::ofstream cache(cacheFile);

    // Zapisywanie danych do pliku cache
    if (cache.is_open()) {
        try {
            cache << json::parse(data).dump(2);
        }
        catch (...) {
            cache << data;
        }
        cache.close();
    }
}

// Funkcja do pobierania danych o wietrze z API dla globalnych wspó³rzêdnych
std::string FetchWindDataGlobal() {
    const char* cacheFile = "global_wind_data_cache.json";

    extern std::vector<std::string> global_latitudes;
    extern std::vector<std::string> global_longitudes;

    // Zwróæ pusty string, jeœli wspó³rzêdne s¹ puste lub ró¿ni¹ siê iloœci¹
    if (global_latitudes.empty() || global_longitudes.empty() || global_latitudes.size() != global_longitudes.size()) {
        return "";
    }

    // Przygotowanie wspó³rzêdnych do requesta
    std::string lat_str, lon_str;
    for (size_t i = 0; i < global_latitudes.size(); ++i) {
        if (i > 0) {
            lat_str += ",";
            lon_str += ",";
        }
        lat_str += global_latitudes[i];
        lon_str += global_longitudes[i];
    }

    std::string start_date = GetFormattedDate(-days_before);
    std::string end_date = GetFormattedDate(days_after);

    // Wysy³anie zapytania do API Open Meteo
    auto response = cpr::Get(cpr::Url{ "https://api.open-meteo.com/v1/forecast" },
        cpr::Parameters{
            {"latitude", lat_str},
            {"longitude", lon_str},
            {"hourly", "wind_direction_180m,wind_direction_120m,wind_direction_80m,"
                      "wind_direction_10m,wind_speed_180m,wind_speed_120m,"
                      "wind_speed_80m,wind_speed_10m,wind_gusts_10m"},
            {"start_date", start_date},
            {"end_date", end_date}
        });

    // Zapisywanie odpowiedzi do cache
    if (response.status_code == 200) {
        CacheWindDataGlobal(response.text);
        return response.text;
    }
    else {
        std::cout << ("Wyst¹pi³ b³¹d podczas pobierania danych z API. Wykorzystany zostanie backupowy plik JSON") << std::endl;

        // Jeœli wyst¹pi³ b³¹d, spróbuj odczytaæ dane z backupowego pliku JSON
        nlohmann::json backup_data;
        const char* backupFile = "backup_wind_data.json";
        std::ifstream backup(backupFile);

        if (backup.is_open()) {
            backup >> backup_data;
            return backup_data.dump();
        }
        else {
            throw std::runtime_error("Nie uda³o siê odczytaæ backupowego pliku JSON");
        }

    }
    throw std::runtime_error("Nie uda³o siê odczytaæ danych z API ani pliku backup");
}

// Funkcja do ³adowania danych o wietrze z cache lub API dla globalnych wspó³rzêdnych
// Naprawiæ error 414 Request-URI Too Large
std::string GetWindDataGlobal() {
    const char* cacheFile = "global_wind_data_cache.json";

    // Jeœli plik cache nie istnieje lub jest starszy ni¿ 1 godzina, pobierz dane z API
    if (!fs::exists(cacheFile) ||
        (fs::file_time_type::clock::now() - fs::last_write_time(cacheFile)) > std::chrono::hours(1)) {
        return FetchWindDataGlobal();
    }


    // Otwórz plik cache
    std::ifstream cache(cacheFile);

    // Jeœli nie uda³o siê otworzyæ pliku cache, pobierz dane z API
    if (!cache.is_open()) {
        return FetchWindDataGlobal();
    }

    // Wczytanie danych z pliku cache
    std::stringstream buffer;
    buffer << cache.rdbuf();
    return buffer.str();
}

// Funkcja do pobierania danych o wietrze dla konkretnych wspó³rzêdnych
// Naprawiæ error 414 Request-URI Too Large
/*
std::string GetWindData(const std::vector<std::string>& latitudes, const std::vector<std::string>& longitudes) {
    if (latitudes.empty() || longitudes.empty() || latitudes.size() != longitudes.size()) {
        throw std::invalid_argument("Przekazano b³êdne wspó³rzêdne geograficzne do funkcji GetWindData");
    }

    // Przygotowanie wspó³rzêdnych do requesta 
    std::string lat_str, lon_str;
    for (size_t i = 0; i < latitudes.size(); ++i) {
        if (i > 0) {
            lat_str += ",";
            lon_str += ",";
        }
        lat_str += latitudes[i];
        lon_str += longitudes[i];
    }

    std::string start_date = GetFormattedDate(-days_before);
    std::string end_date = GetFormattedDate(days_after);

    // Wysy³anie zapytania do API Open Meteo
    auto response = cpr::Get(cpr::Url{ "https://api.open-meteo.com/v1/forecast" },
        cpr::Parameters{
            {"latitude", lat_str},
            {"longitude", lon_str},
            {"hourly", "wind_direction_180m,wind_direction_120m,wind_direction_80m,"
                      "wind_direction_10m,wind_speed_180m,wind_speed_120m,"
                      "wind_speed_80m,wind_speed_10m,wind_gusts_10m"},
            {"start_date", start_date},
            {"end_date", end_date}
        });

    std::cout << "Request URL:" << response.url << std::endl;
	std::cout << "Response Status Code: " << response.status_code << std::endl;
    // Zapisywanie odpowiedzi do cache
    if (response.status_code == 200) {
        return response.text;
    }
    throw std::runtime_error("Wyst¹pi³ b³¹d podczas pobierania danych z API");
}
*/

// Funkcja do pobierania danych o wietrze dla konkretnych wspó³rzêdnych (BATCH) Maks. d³ugoœæ URL ~6700 znaków
std::string GetWindData(const std::vector<std::string>& latitudes, const std::vector<std::string>& longitudes) {
    if (latitudes.empty() || longitudes.empty() || latitudes.size() != longitudes.size()) {
        throw std::invalid_argument("Przekazano bledne wspolrzedne geograficzne do funkcji GetWindData");
    }
    size_t max_url_length = 6700;
    const std::string base_url = "https://api.open-meteo.com/v1/forecast";
    const std::string hourly_param =
        "wind_direction_180m,wind_direction_120m,wind_direction_80m,"
        "wind_direction_10m,wind_speed_180m,wind_speed_120m,"
        "wind_speed_80m,wind_speed_10m,wind_gusts_10m";
    std::string start_date = GetFormattedDate(-days_before);
    std::string end_date = GetFormattedDate(days_after);

    nlohmann::json full_json = nlohmann::json::array();
    size_t n = latitudes.size();
    size_t i = 0;


    while (i < n) {
        // Budujemy batch punktów
        std::string lat_str;
        std::string lon_str;
        size_t batch_start = i;

        for (; i < n; ++i) {
            const std::string& next_lat = latitudes[i];
            const std::string& next_lon = longitudes[i];

            std::string temp_lat = lat_str.empty() ? next_lat : lat_str + "," + next_lat;
            std::string temp_lon = lon_str.empty() ? next_lon : lon_str + "," + next_lon;

            // Tworzenie URL do sprawdzenia d³ugoœci
            std::string test_url = base_url
                + "?latitude=" + temp_lat
                + "&longitude=" + temp_lon
                + "&hourly=" + hourly_param
                + "&start_date=" + start_date
                + "&end_date=" + end_date;

            // Jeœli przekroczono limit, to nie dodawaj tego punktu do batcha
            if (test_url.size() > max_url_length) {
                break;
            }

            lat_str = std::move(temp_lat);
            lon_str = std::move(temp_lon);
        }

        // Wysy³amy request dla aktualnego batcha
        auto response = cpr::Get(
            cpr::Url{ base_url },
            cpr::Parameters{
                {"latitude", lat_str},
                {"longitude", lon_str},
                {"hourly", hourly_param},
                {"start_date", start_date},
                {"end_date", end_date}
            }
        );


        if (response.status_code != 200) {
            std::cout << response.status_code << (" - Wystapil blad podczas pobierania danych z API. Wykorzystany zostanie backupowy plik JSON") << std::endl;

            // Jeœli wyst¹pi³ b³¹d, spróbuj odczytaæ dane z backupowego pliku JSON
            nlohmann::json backup_data;
            const char* backupFile = "backup_wind_data.json";
            std::ifstream backup(backupFile);

            if (backup.is_open()) {
                backup >> backup_data;
                return backup_data.dump();
            }
            else {
                throw std::runtime_error("Nie udalo sie odczytac danych z API ani pliku backup");
            }
        }

        nlohmann::json batch_json = nlohmann::json::parse(response.text);
        if (!batch_json.is_array()) {
            throw std::runtime_error("Oczekiwano pliku JSON w responsie");
        }
        for (auto& element : batch_json) {
            full_json.push_back(element);
        }
    }

    std::cout << "Ilosc requestow: " << i << std::endl;

    std::string output_filename = "wind_data_cache.json";
    std::ofstream cache(output_filename);
    if (!cache.is_open()) {
        throw std::runtime_error("Nie udalo sie otworzyc pliku JSON do zapisu danych");
    }
    cache << full_json.dump(2);
    cache.close();

    return full_json.dump(2);
}

/*
// Funkcja do cachowania danych o wietrze dla konkretnych wspó³rzêdnych
void CacheWindData(const std::string& data,
    const std::vector<std::string>& latitudes,
    const std::vector<std::string>& longitudes)
{
    const char* cacheFile = "wind_data_cache.json";
    json cache_data;

    // Wczytaj istniej¹ce dane z cache, jeœli istniej¹
    if (fs::exists(cacheFile)) {
        std::ifstream existing(cacheFile);
        if (existing.is_open()) {
            try {
                existing >> cache_data;
            }
            catch (...) {
                cache_data = json::object(); // Jeœli nie uda³o siê sparsowaæ, utwórz nowy obiekt
            }
            existing.close();
        }
    }

    // Parsuj nowe dane
    json new_data = json::parse(data);

    // Dla ka¿dej pary wspó³rzêdnych
    for (size_t i = 0; i < latitudes.size(); ++i) {
        // Tworzenie klucza dla pary wspó³rzêdnych
        std::string key = latitudes[i] + "_" + longitudes[i];

        // Zapisz lub nadpisz dane dla tych wspó³rzêdnych
        cache_data[key] = new_data;
    }

    // Zapisz zaktualizowane dane do pliku
    std::ofstream cache(cacheFile);
    if (cache.is_open()) {
        cache << cache_data.dump(2);
        cache.close();
    }
}

// Funkcja do pobierania danych o wietrze z API dla konkretnych wspó³rzêdnych
std::string FetchWindData(const std::vector<std::string>& latitudes, const std::vector<std::string>& longitudes) {
    // Zwróæ pusty string, jeœli wspó³rzêdne s¹ puste lub ró¿ni¹ siê iloœci¹
    if (latitudes.empty() || longitudes.empty() || latitudes.size() != longitudes.size()) {
        return "";
    }

    // Przygotowanie wspó³rzêdnych do requesta 
    std::string lat_str, lon_str;
    for (size_t i = 0; i < latitudes.size(); ++i) {
        if (i > 0) {
            lat_str += ",";
            lon_str += ",";
        }
        lat_str += latitudes[i];
        lon_str += longitudes[i];
    }

    std::string start_date = GetFormattedDate(-days_before);
    std::string end_date = GetFormattedDate(days_after);

    // Wysy³anie zapytania do API Open Meteo
    auto response = cpr::Get(cpr::Url{ "https://api.open-meteo.com/v1/forecast" },
        cpr::Parameters{
            {"latitude", lat_str},
            {"longitude", lon_str},
            {"hourly", "wind_direction_180m,wind_direction_120m,wind_direction_80m,"
                      "wind_direction_10m,wind_speed_180m,wind_speed_120m,"
                      "wind_speed_80m,wind_speed_10m,wind_gusts_10m"},
            {"start_date", start_date},
            {"end_date", end_date}
        });

    // Zapisywanie odpowiedzi do cache
    if (response.status_code == 200) {
        CacheWindData(response.text, latitudes, longitudes);
        return response.text;
    }
    return "";
}
*/

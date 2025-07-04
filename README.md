# Wizualizacja wiatrów w OpenGL

![Nagranie ekranu z aplikacji](./nagranie_ekranu.gif)

## Opis projektu

Aplikacja umożliwia interaktywną wizualizację globalnych wzorców wiatru na trójwymiarowym modelu kuli ziemskiej. Projekt prezentuje dane meteorologiczne w przystępny sposób, wykorzystując przy tym wiele technik grafiki komputerowej.
Aplikacja pozwala na odczyt kierunków i prędkości wiatrów na całym świecie na podstawie historycznych danych, korzystając z API NOMADS (https://nomads.ncep.noaa.gov/) dostarczonego przez National Oceanic and Atmospheric Administration.

Dzięki prostemu interfejsowi wraz z samouczkiem oraz graficznym elementom, takim jak animowane linie wiatru oraz strzałki z dominującymi kierunkami wiatru, aplikacja pozwala w łatwy sposób analizować i interpretować zjawiska atmosferyczne.

## Główne funkcje aplikacji

- Animowana wizualizacja globalnych wiatrów na podstawie danych z API.
- Możliwość kliknięcia w wybrany region i wyświetlenia dominujących kierunków wiatru.
- Wbudowany samouczek wyjaśniający jak korzystać z aplikacji oraz jak interpretować wizualizację.
- Możliwość ukrywania i wyświetlania poszczególnych warstw.
- Możliwość zmiany daty danych o wietrze.

## Elementy aplikacji

### 1. Trójwymiarowa wizualizacja Ziemi

- Realistyczny model planety z wysokiej jakości teksturami satelitarnymi
- Mapa normalnych do renderowania szczegółów powierzchni
- Efekt atmosfery z niebieską poświatą wokół planety
- Skybox tworzący tło kosmiczne

### 2. Wizualizacja wiatrów

- Animowane linie wiatru wizualizujące kierunki i prędkości wiatrów
- Nakładka mapująca prędkości wiatru na kolory (od niebieskiego do zielonego)
- Strzałki reprezentujące przeważający kierunek wiatru w danym państwie

### 3. Interakcja z danymi geograficznymi

- Rysowanie granic państw ładowanych z plików Shapefile
- Interaktywne wybieranie krajów poprzez kliknięcie
- Podświetlanie granic wybranego kraju

### 4. Dane meteorologiczne

- Dane pobierane z API NOMADS
- Format GRIB konwertowany do JSON
- Przeliczanie parametrów wiatru w danych: UGRD (składowa U wiatru), VGRD (składowa V wiatru), GUST (porywy wiatru)
- Rozdzielczość danych: 1° x 1° (około 111 km szerokości na kafelek)
- System cache'owania danych dla przyspieszenia uruchamiania aplikacji
- Tryb offline z danymi backupowymi

### 5. Interfejs Użytkownika

- Menu szybkich ustawień
- Legenda ze skalą prędkości wiatru
- Samouczek po uruchomieniu aplikacji
- Selektor dat umożliwiający wybieranie danych historycznych do 7 dni wstecz
- Możliwość zmiany warstw (mapa satelitarna, nakładka prędkości wiatru, linie wiatru)

### 6. Sterowanie

- Kamera orbitalna z możliwością obrotu wokół planety
- Kontrola kamery klawiaturą i myszką (obrót i zoomowanie)

<br>

Instrukcja krok po kroku jak zainstalować bibliotekę `libcurl` w Visual Studio w środowisku Windows z użyciem `vcpkg`.

Na początku upewnij się, że w Visual Studio → Project → Project Properties → C/C++ → Language - Jest ustawiony **Standard ISO C++ 17**

## 1️. Instalacja `vcpkg`

### a) Klonowanie repozytorium

Otwórz terminal (np. PowerShell) i wpisz:

```powershell
git clone https://github.com/microsoft/vcpkg
```

### b) Przejście do katalogu

```powershell
cd vcpkg
```

### c) Budowanie vcpkg

```powershell
.\bootstrap- vcpkg.bat
```

## 2. Integracja vcpkg z Visual Studio

Aby vcpkg współpracował automatycznie z Visual Studio, wykonaj:

```powershell
.\vcpkg integrate install
```

Dzięki temu Visual Studio automatycznie wykryje wszystkie biblioteki zainstalowane przez vcpkg.

## 3. Instalacja bibliotek

Podstawowa instalacja libcurl:

```powershell
.\vcpkg install curl:x86- windows
```

Instalacja cpr:

```powershell
.\vcpkg install cpr:x86- windows
```

## 4. Ustawienie prawidłowego kompilatora

Jeśli masz np. `vcpkg install curl:x86- windows`, a w Visual Studio masz ustawioną konfigurację `Wi64`, to Visual Studio nie widzi pakietów `x86- windows`.

Rozwiązanie:

- W Visual Studio:
  Build → Configuration Manager → zmień platformę na x86.

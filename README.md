# Projekt z Grafiki Komputerowej

## Projekt - Róża Wiatrów i Wzorce Wiatru

Cel: Nałóż animowane wzorce wiatru na mapę Europy używając glifów (strzałek). Uwzględnij interaktywne róże wiatrów pokazujące dominujące kierunki dla regionu po kliknięciu. Interaktywny wybór historycznych danych.

Techniki GK: Wizualizacja pól wektorowych (instancjonowane strzałki/glify), Animacja oparta na shaderach (np. pulsowanie/kolor), Mapowanie cieni, Podstawowy interfejs użytkownika/interakcja, Przetwarzanie danych (dane o wietrze, np. z Ventusky używając nowcastingu).

Specyfikacja: Reprezentuj wektory wiatru za pomocą instancjonowanych glifów strzałek 2D/3D. Animuj je subtelnie za pomocą shadera (np. pulsowanie koloru, lekki ruch). Wyświetlanie róży wiatrów po interakcji wymaga elementów interfejsu i agregacji danych regionalnych.

## Prezentacja - Explosion with geometry shader

https://learnopengl.com/Advanced-OpenGL/Geometry-Shader
https://medium.com/chenjd-xyz/using-the-geometry-shader-to-achieve-model-explosion-effect-cf6d5ec03020

---

<br>

# Instalacja `libcurl` w Visual Studio za pomocą `vcpkg`

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
.\bootstrap-vcpkg.bat
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
.\vcpkg install curl:x86-windows
```

Instalacja cpr:

```powershell
.\vcpkg install cpr:x86-windows
```

## 4. Ustawienie prawidłowego kompilatora

Jeśli masz np. `vcpkg install curl:x86-windows`, a w Visual Studio masz ustawioną konfigurację:

- Wi64 — wtedy VS nie widzi pakietów x86-windows.

Rozwiązanie:

- W Visual Studio:
  Build → Configuration Manager → zmień platformę na x86.

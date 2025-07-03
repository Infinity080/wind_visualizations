#include "glew.h"
#include <GLFW/glfw3.h>
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <algorithm>

#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Texture.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include "shapefil.h"
#include "SOIL/SOIL.h"

#include "Get_Wind_Data.h"
#include "imgui_internal.h"

#include <nlohmann/json.hpp>



///////// Inicjalizacje zmiennych //////////
// Inicjalizacja zmiennych dla tekstur
namespace texture {
	GLuint earth;
	GLuint earthNormal;
	GLuint skybox;
}

// Inicjalizacja zmiennych dla siatki
struct Grid {
	std::vector<float> latitudes;   // Lista szerokości geograficznych środków kafelków
	std::vector<float> longitudes;  // Lista długości geograficznych środków kafelków
	std::vector<float> windAngles; // Lista kierunków wiatru w stopniach
	std::vector<float> windSpeeds; // Lista prędkości wiatru w m/s
	size_t numTiles; // Liczba kafelków w siatce
};

struct CountryWindInfo {
	float avgLat = 0.0f;
	float avgLon = 0.0f;
	float avgAngle = 0.0f;
	float avgSpeed = 0.0f;
};

struct Country {
	int id;
	std::string name;
	std::vector<std::vector<glm::vec3>> boundaries;
};

std::vector<Country> countries;

int selectedCountryId = -1;
glm::vec3 borderColor = glm::vec3(0.0f, 0.0f, 2.55f);

Grid grid;

float gridTileSize = 11.0f; // Rozmiar pojedynczego kafelka siatki

// Zmienne dla linii wiatru
struct WindLineData {
	glm::vec3 startPos;     // Pozycja początkowa linii
	glm::vec3 direction;    // Kierunek wiatru (znormalizowany)
	float speed;            // Prędkość wiatru
	float length;           // Długość linii
};

GLuint selectedCountryVAO = 0, selectedCountryVBO = 0;
std::vector<GLuint> selectedCountryFirstVert, selectedCountryVertCount;

GLuint windLinesVAO, windLinesVBO, windLinesInstanceVBO;
std::vector<WindLineData> windLineData;
std::vector<glm::mat4> windLineMatrices;

const int LINES_PER_POINT = 1; // Liczba linii na punkt siatki
const float LINE_LENGTH = 2.0f; // Długość linii wiatru
const float LINE_THICKNESS = 0.25f; // Grubość linii wiatru

// Inicjalizacja zmiennych dla programow shaderow
GLuint program; // Podstawowy program do rysowania kolorem
GLuint programTex; // Program do rysowania z teksturą
GLuint programAtm; // Program do rysowania atmosfery
GLuint programOverlay; // Program do rysowania nakładki prędkości wiatru
GLuint programWindLines; // Program do rysowania linii wiatru
GLuint programSkybox; // Program do rysowania skyboxa
GLuint programPointsInstanced; // Program do rysowania instancjonowanych punktów

Core::Shader_Loader shaderLoader;

Core::RenderContext sphereContext;
Core::RenderContext arrowContext;
Core::RenderContext skyboxContext;
Core::RenderContext atmosphereContext;

GLuint arrowInstanceVBO; // VBO dla danych instancji strzałek (macierze modelu)
std::vector<glm::mat4> arrowModelMatrices; // Wektor macierzy modelu dla każdej strzałki

// Zmienne kamery
glm::vec3 cameraPos = glm::vec3(-6.f, 0, 0);
glm::vec3 cameraDir = glm::vec3(1.f, 0.f, 0.f);
float aspectRatio = 1.f;
float angleSpeed = 0.01f;
float moveSpeed = 0.01f;

float cameraAngleX = -0.25f;
float cameraAngleY = 0.8f;
float cameraDistance = 30.0f;

bool dragging = false;
double lastX = 0.0;
double lastY = 0.0;
float mouseSensitivity = 0.005f;

GLuint pointsVAO = 0, pointsVBO = 0, pointsInstanceVBO = 0;
GLuint bordersVAO = 0, bordersVBO = 0;
std::vector<GLuint> countryFirstVert, countryVertCount;
std::vector<glm::vec3> pointPositions;
std::vector<glm::vec3> pointColors;

float planetRadius = 3.0f;
float modelRadius = 110.0f;

glm::vec3 planetScale = glm::vec3(planetRadius / modelRadius);
glm::mat4 planetModelMatrix = glm::scale(glm::mat4(1.0f), planetScale);


float arrowSize = 0.02f;
float arrowScaleModel = arrowSize * modelRadius / planetRadius;

std::string windDataGlobal; // Globalne dane o wietrze

CountryWindInfo calculateCountryWindInfo(int countryId);

// Zmienne QuickMenu
int cameraSpeed = 100;
int animationSpeed = 100;
bool show_overlay = true;
bool show_earthmap = true;
bool show_tutorial = true;
bool show_wind_arrows = true;
int daysBefore = 0;
bool isQuickMenuOpen = false;
GLuint clock_icon = 0;
GLuint move_icon = 0;
GLuint overlay_icon = 0;
GLuint earthmap_icon = 0;
GLuint tutorial_icon = 0;
GLuint date_icon = 0;
GLuint windarrows_icon = 0;
float ImGuiWidth = 460.0f;
float ImGuiHeight = 380.0f;

std::string date = GetFormattedDate(-daysBefore); // Dzisiejsza data

bool isUpdating = false;

// Zmienne dla nakładki
GLuint overlayVAO, overlayVBO, overlayEBO;
std::vector<unsigned int> overlayIndices;
float maxWindSpeed = 32.6f; // Maksymalna prędkość wiatru do normalizacji kolorów
std::string max_speed_str = std::to_string(static_cast<int>(maxWindSpeed));

// Zmienne dla tutorialu
static bool tutorial_popup_open = false;
static int tutorial_step = -1;
static bool tutorial_steps_initialized[4] = { false, false, false, false };
float rotationSpeed = 0.0002f; // Prędkość obrotu kamery w tutorialu
bool revertCamera = false;
bool zoomCamera = false;
float targetCameraDistance = 6.0f;
float targetCameraAngleY = 0.8f;

glm::vec3 latLonToXYZ(float latInput, float lonInput) {
	float lat = glm::radians(latInput);
	float lon = glm::radians(lonInput);
	float x = cos(lat) * cos(lon);
	float y = sin(lat);
	float z = -cos(lat) * sin(lon);
	return glm::vec3(x, y, z);
}

////////////////////////////////////////////////////
	// Quick Guide for navigating through shapefiles:
	/*
	* We have shapes -> parts inside shapes -> vertices
	* shapefile contains number of shapes, shape type and a bounding box
	* SHPObject:
		* panPartStart - indexes for padfX,padfY
		* nParts - number of parts of a shape
		* nVertices - number of vertices of a shape
		* padfX,padfY - actual coordinates (arrays)
	*/
void loadCountryBoundaries(const std::string& filePath) {
	SHPHandle handler = SHPOpen(filePath.c_str(), "rb");

	DBFHandle dbfHandler = DBFOpen(filePath.c_str(), "rb");

	int nameFieldIndex = DBFGetFieldIndex(dbfHandler, "NAME");

	int nEntities;
	int shapeType;
	double bounds1[4], bounds2[4];
	SHPGetInfo(handler, &nEntities, &shapeType, bounds1, bounds2);
	std::cout << "Typ geometrii w pliku SHP: " << shapeType << std::endl;

	countries.clear();

	std::map<std::string, int> countryNameToIndex;

	for (int i = 0; i < nEntities; ++i) {
		SHPObject* shpObj = SHPReadObject(handler, i);
		if (!shpObj) continue;

		const char* name = DBFReadStringAttribute(dbfHandler, i, nameFieldIndex);
		std::string countryName = name ? name : "Unknown";

		int countryIndex;
		if (countryNameToIndex.count(countryName)) {
			countryIndex = countryNameToIndex[countryName];
		}
		else {
			Country newCountry;
			newCountry.id = static_cast<int>(countries.size());
			newCountry.name = countryName;
			countries.push_back(newCountry);
			countryIndex = newCountry.id;
			countryNameToIndex[countryName] = countryIndex;
		}

		for (int part = 0; part < shpObj->nParts; ++part) {
			int start = shpObj->panPartStart[part];
			int end = (part + 1 < shpObj->nParts) ? shpObj->panPartStart[part + 1] : shpObj->nVertices;

			std::vector<glm::vec3> boundary;
			for (int j = start; j < end; ++j) {
				double lon = shpObj->padfX[j];
				double lat = shpObj->padfY[j];
				glm::vec3 coords = latLonToXYZ(lat, lon);
				boundary.emplace_back(coords);
			}

			if (boundary.size() > 1) {
				countries[countryIndex].boundaries.push_back(std::move(boundary));
			}
		}

		SHPDestroyObject(shpObj);
	}

	DBFClose(dbfHandler);
	SHPClose(handler);
}

//////// Funkcje do kamery //////////
glm::mat4 createCameraMatrix()
{
	float maxAngleY = glm::radians(89.0f);
	cameraAngleY = glm::clamp(cameraAngleY, -maxAngleY, maxAngleY);

	float x = cameraDistance * cos(cameraAngleY) * cos(cameraAngleX);
	float y = cameraDistance * sin(cameraAngleY);
	float z = cameraDistance * cos(cameraAngleY) * sin(cameraAngleX);
	cameraPos = glm::vec3(x, y, z);
	cameraDir = glm::normalize(-cameraPos);
	return glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
}

glm::mat4 createPerspectiveMatrix()
{
	glm::mat4 perspectiveMatrix;
	float n = 0.05;
	float f = 80.;
	float a1 = glm::min(aspectRatio, 1.f);
	float a2 = glm::min(1 / aspectRatio, 1.f);
	perspectiveMatrix = glm::mat4({
		1,0.,0.,0.,
		0.,aspectRatio,0.,0.,
		0.,0.,(f + n) / (n - f),2 * f * n / (n - f),
		0.,0.,-1.,0.,
		});
	perspectiveMatrix = glm::transpose(perspectiveMatrix);
	return perspectiveMatrix;
}

glm::vec3 getRayFromMouse(double mouseX, double mouseY, int screenWidth, int screenHeight) {
	float x = (2.0f * static_cast<float>(mouseX)) / screenWidth - 1.0f;
	float y = 1.0f - (2.0f * static_cast<float>(mouseY)) / screenHeight;
	float z = 1.0f;

	glm::vec3 rayNDC(x, y, z);

	glm::vec4 rayClip(rayNDC.x, rayNDC.y, -1.0f, 1.0f);

	glm::mat4 projection = createPerspectiveMatrix();
	glm::vec4 rayEye = glm::inverse(projection) * rayClip;
	rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

	glm::mat4 view = createCameraMatrix();
	glm::vec3 rayWorld = glm::vec3(glm::inverse(view) * rayEye);
	rayWorld = glm::normalize(rayWorld);

	return rayWorld;
}

void revertCameraXToDefault() {
	const float targetAngleX = -0.25f;
	const float smoothingSpeed = 0.02f;

	float difference = targetAngleX - cameraAngleX;

	if (std::abs(difference) < 0.1f) {
		revertCamera = false;
		return;
	}

	cameraAngleX += difference * smoothingSpeed;
	
}

void zoomCameraToDefault() {
	const float smoothingSpeed = 0.03f;

	float distanceDiff = targetCameraDistance - cameraDistance;
	float angleYDiff = targetCameraAngleY - cameraAngleY;

	if (std::abs(distanceDiff) < 0.1f && std::abs(angleYDiff) < 0.1f) {
		zoomCamera = false;
		return;
	}

	cameraDistance += distanceDiff * smoothingSpeed;
	cameraAngleY += angleYDiff * smoothingSpeed;
}
// Funkcja do kontroli scrolla
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse) {
		return;
	}
	cameraDistance -= yoffset * moveSpeed * 5.0f;
	cameraDistance = glm::clamp(cameraDistance, 4.0f, 8.0f);
}

////////////////////////////////////////////////////


///////// Funkcje do rysowania /////////

// Funkcja do rysowania obiektu z jednolitym kolorem
void drawObjectColor(Core::RenderContext& context, glm::mat4 modelMatrix, glm::vec3 color) {
	GLuint prog = program;
	glUseProgram(prog);

	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(prog, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(prog, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(prog, "color"), color.x, color.y, color.z);
	glUniform3f(glGetUniformLocation(prog, "lightPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	glUniform3f(glGetUniformLocation(prog, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	Core::DrawContext(context);
	glUseProgram(0);
}


void drawSkybox() {
	// Wyłączenie zapisu głębokości i włączenie tylko odczytu
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);

	GLuint prog = programSkybox;
	glUseProgram(prog);

	// Macierz widoku bez translacji
	glm::mat4 view = glm::mat4(glm::mat3(createCameraMatrix()));
	glm::mat4 projection = createPerspectiveMatrix();

	glUniformMatrix4fv(glGetUniformLocation(prog, "view"), 1, GL_FALSE, (float*)&view);
	glUniformMatrix4fv(glGetUniformLocation(prog, "projection"), 1, GL_FALSE, (float*)&projection);

	// Ustawienie tekstury skyboxa
	Core::SetActiveTexture(texture::skybox, "skybox", prog, 0);

	// Rysowanie skyboxa (jako duża kula)
	Core::DrawContext(skyboxContext);

	// Przywrócenie ustawień głębokości
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glUseProgram(0);
}


void drawArrow(float latInput, float lonInput, float rotation, glm::vec3 color) {
	glm::vec3 normal = glm::normalize(latLonToXYZ(latInput, lonInput)); // wektor normalny do planety
	glm::vec3 point = normal * (modelRadius + 1.0f); // punkt strzałki

	glm::vec3 Y = glm::normalize(glm::cross(glm::vec3(0, 1, 0), normal));
	glm::vec3 X = glm::normalize(glm::cross(normal, Y));

	float alpha = glm::radians(rotation);
	glm::vec3 rot = glm::normalize(X * glm::cos(alpha) + Y * glm::sin(alpha)); // wzór na rotację w płaszczyźnie

	glm::mat4 modelMatrix = planetModelMatrix
		* glm::translate(glm::mat4(1), point) // przesunięcie
		* glm::mat4(glm::rotation(glm::vec3(0, 0, 1), rot)) // rotacja za pomocą kwaternionu
		* glm::scale(glm::mat4(1), glm::vec3(arrowScaleModel)); // skalowanie

	drawObjectColor(arrowContext, modelMatrix, color);
}


// Funkcja do rysowania obiektu z teksturą
void drawObjectTexture(Core::RenderContext& context, glm::mat4 modelMatrix, GLuint colorTextureID, GLuint normalMapTextureID) {
	GLuint prog = programTex;
	glUseProgram(prog);
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniform3f(glGetUniformLocation(prog, "lightPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	glUniformMatrix4fv(glGetUniformLocation(prog, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(prog, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(prog, "lightPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	glUniform3f(glGetUniformLocation(prog, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	Core::SetActiveTexture(colorTextureID, "colorTexture", prog, 0);
	Core::SetActiveTexture(normalMapTextureID, "normalMap", prog, 1);

	Core::DrawContext(context);
	glUseProgram(0);
}

// Funkcja do rysowania atmosfery
void drawObjectAtmosphere(Core::RenderContext& context, glm::mat4 modelMatrix) {
	GLuint prog = programAtm;
	glUseProgram(prog);

	// Włączenie blendingu addytywnego
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE); // Kolory będą dodawane
	glDepthMask(GL_FALSE);      // Nie zapisuj do bufora głębokosci, aby nie zasłonić planety

	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 scaledModelMatrix = modelMatrix * glm::scale(glm::mat4(1.0f), glm::vec3(1.025f));
	glm::mat4 transformation = viewProjectionMatrix * scaledModelMatrix;

	glUniformMatrix4fv(glGetUniformLocation(prog, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(prog, "modelMatrix"), 1, GL_FALSE, (float*)&scaledModelMatrix);
	glUniform3f(glGetUniformLocation(prog, "lightPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	glUniform3f(glGetUniformLocation(prog, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	// Uniformy dla atmosfery
	glUniform3f(glGetUniformLocation(prog, "atmosphereColor"), 0.35f, 0.57f, 1.0f); // Kolor poświaty
	glUniform1f(glGetUniformLocation(prog, "intensity"), 1.5f); // Intensywnosc poświaty

	Core::DrawContext(context);

	// Wyłączenie blendingu i przywrócenie zapisu do bufora głębokosci
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	glUseProgram(0);
}

// Funkcja do aktualizacji danych o strzałkach wiatru dla renderingu instancyjnego

void updateWindArrowData() {
	windLineData.clear();
	windLineMatrices.clear();

	if (grid.numTiles == 0) {
		return;
	}

	try {
		windLineData.reserve(grid.numTiles * LINES_PER_POINT);
		windLineMatrices.reserve(grid.numTiles * LINES_PER_POINT);

		for (size_t i = 0; i < grid.numTiles; ++i) {
			try {
				float lat = grid.latitudes[i];
				float lon = grid.longitudes[i];
				float windAngle = grid.windAngles[i];
				float windSpeed = grid.windSpeeds[i];

				// Oblicz pozycję na powierzchni planety
				glm::vec3 normal = glm::normalize(latLonToXYZ(lat, lon));
				glm::vec3 basePoint = normal * (modelRadius + 0.01f);

				// Oblicz kierunek wiatru w lokalnym układzie współrzędnych
				glm::vec3 Y = glm::normalize(glm::cross(glm::vec3(0, 1, 0), normal));
				glm::vec3 X = glm::normalize(glm::cross(normal, Y));
				float alpha = glm::radians(windAngle);
				glm::vec3 windDirection = glm::normalize(X * glm::cos(alpha) + Y * glm::sin(alpha));

				// Utwórz kilka linii dla tego punktu z różnymi offsetami
				for (int lineIdx = 0; lineIdx < LINES_PER_POINT; ++lineIdx) {
					WindLineData lineData;

					// Różne offsety dla każdej linii
					float lineOffset = (lineIdx - LINES_PER_POINT / 2.0f) * 0.002f;
					glm::vec3 offsetPos = basePoint + normal * lineOffset;

					lineData.startPos = offsetPos;
					lineData.direction = windDirection;
					lineData.speed = windSpeed;
					lineData.length = LINE_LENGTH * (0.8f + 0.4f * (lineIdx + 1) / LINES_PER_POINT);

					windLineData.push_back(lineData);

					// Utwórz macierz transformacji dla linii
					// Najpierw stwórz macierz rotacji, aby linia była skierowana w kierunku wiatru
					glm::vec3 up = normal;
					glm::vec3 right = glm::normalize(glm::cross(windDirection, up));
					glm::vec3 forward = windDirection;

					glm::mat4 rotationMatrix = glm::mat4(1.0f);
					rotationMatrix[0] = glm::vec4(forward, 0.0f);
					rotationMatrix[1] = glm::vec4(right, 0.0f);
					rotationMatrix[2] = glm::vec4(up, 0.0f);

					glm::mat4 modelMatrix = planetModelMatrix
						* glm::translate(glm::mat4(1), lineData.startPos)
						* rotationMatrix
						* glm::scale(glm::mat4(1), glm::vec3(lineData.length, LINE_THICKNESS, LINE_THICKNESS));

					windLineMatrices.push_back(modelMatrix);
				}
			}
			catch (const std::exception& e) {
				// Jeśli wystąpi błąd podczas przetwarzania konkretnego punktu, pomijamy go
			}
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Blad podczas przetwarzania danych o wietrze: " << e.what() << std::endl;
		return;
	}

	// Aktualizacja VBO z macierzami modeli
	if (!windLineMatrices.empty()) {
		glBindBuffer(GL_ARRAY_BUFFER, windLinesInstanceVBO);
		glBufferData(GL_ARRAY_BUFFER, windLineMatrices.size() * sizeof(glm::mat4), windLineMatrices.data(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

// Funkcja do rysowania animowanych linii wiatru
void drawWindArrows() {
	if (windLineMatrices.empty()) {
		std::cout << "Brak danych linii wiatru do narysowania" << std::endl;
		return;
	}

	// Włączenie blendingu dla przezroczystości
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	GLuint prog = programWindLines;
	glUseProgram(prog);

	// Pobierz czas dla animacji z wyższą precyzją
	float time = static_cast<float>(glfwGetTime());
	float animSpeed = animationSpeed / 100.0f;

	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glUniformMatrix4fv(glGetUniformLocation(prog, "viewProjectionMatrix"), 1, GL_FALSE, (float*)&viewProjectionMatrix);
	glUniform3f(glGetUniformLocation(prog, "baseColor"), 1.0f, 1.0f, 1.0f);
	glUniform1f(glGetUniformLocation(prog, "time"), time * animSpeed);
	glUniform1f(glGetUniformLocation(prog, "maxWindSpeed"), maxWindSpeed);
	// Przekaż dane o prędkości wiatru
	std::vector<float> speedData;
	speedData.reserve(windLineData.size());
	for (const auto& lineData : windLineData) {
		speedData.push_back(lineData.speed);
	}

	GLuint speedVBO;
	glGenBuffers(1, &speedVBO);
	glBindBuffer(GL_ARRAY_BUFFER, speedVBO);
	glBufferData(GL_ARRAY_BUFFER, speedData.size() * sizeof(float), speedData.data(), GL_DYNAMIC_DRAW);

	glBindVertexArray(windLinesVAO);

	glEnableVertexAttribArray(7);
	glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
	glVertexAttribDivisor(7, 1);

	// Rysuj linie jako instancjonowane prostopadłościany
	glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, windLineMatrices.size());

	glBindVertexArray(0);
	glDeleteBuffers(1, &speedVBO);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glUseProgram(0);
}

double calculateWindAngle(double u, double v) {
	const double PI = 3.14159265358979323846;
	double angleRad = atan2(u, v); // Obliczamy kierunek strzałki w radianach
	double angleDeg = angleRad * 180.0 / PI;
	if (angleDeg < 0) angleDeg += 360.0;
	return angleDeg;
}

void drawWindOverlay() {
	if (overlayIndices.empty()) {
		return;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	GLuint prog = programOverlay;
	glUseProgram(prog);

	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * planetModelMatrix * glm::scale(glm::vec3(1.001f)); // Lekkie skalowanie, aby uniknąć z-fightingu
	glUniformMatrix4fv(glGetUniformLocation(prog, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniform1f(glGetUniformLocation(prog, "maxWindSpeed"), maxWindSpeed);

	glBindVertexArray(overlayVAO);
	glDrawElements(GL_TRIANGLES, overlayIndices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glUseProgram(0);
}
////////////////////////////////////////////////////

///////// Funkcje do siatki //////////
// Funkcja do tworzenia siatki składającej się z kafelków
Grid createGrid() {
	std::string cacheFileName = date + "_grid.dat";

	if (std::filesystem::exists(cacheFileName)) {
		std::cout << "Ladowanie siatki z pliku: " << cacheFileName << std::endl;
		Grid grid;
		std::ifstream inFile(cacheFileName, std::ios::binary);
		if (inFile.is_open()) {
			inFile.read(reinterpret_cast<char*>(&grid.numTiles), sizeof(grid.numTiles));

			auto read_vector = [&](std::vector<float>& vec) {
				size_t size;
				inFile.read(reinterpret_cast<char*>(&size), sizeof(size));
				vec.resize(size);
				inFile.read(reinterpret_cast<char*>(vec.data()), size * sizeof(float));
				};

			read_vector(grid.latitudes);
			read_vector(grid.longitudes);
			read_vector(grid.windAngles);
			read_vector(grid.windSpeeds);

			inFile.close();
			std::cout << "Zakonczono ladowanie siatki z pliku." << std::endl;
			return grid;
		}
		else {
			std::cerr << "Nie mozna otworzyc pliku do odczytu: " << cacheFileName << std::endl;
		}
	}

	Grid grid;
	grid.numTiles = 0;
	std::cout << "Rozpoczeto tworzenie siatki" << std::endl;
	if (windDataGlobal.empty()) {
		std::cerr << "Brak danych o wietrze do utworzenia siatki." << std::endl;
		return grid;
	}

	try {
		auto jsonData = nlohmann::json::parse(windDataGlobal);
		std::set<std::pair<float, float>> uniqueCoords;

		// Tymczasowe mapowanie współrzędnych na U, V, GUST
		std::map<std::pair<float, float>, float> uMap;
		std::map<std::pair<float, float>, float> vMap;
		std::map<std::pair<float, float>, float> gustMap;

		// Parsowanie pliku JSON z danymi o wietrze
		for (const auto& item : jsonData) {
			if (item.contains("latitude") && item.contains("longitude") && item.contains("parameter") && item.contains("value")) {
				float lat = item["latitude"].get<float>();
				float lon = item["longitude"].get<float>();
				std::string param = item["parameter"];
				float value = item["value"].get<float>();

				// Zapisywanie odczytanych współrzędnych
				uniqueCoords.insert({ lat, lon });

				// Zapisywanie odczytanych parametrów do tymczasowych zmiennych
				if (param == "UGRD") uMap[{lat, lon}] = value;
				else if (param == "VGRD") vMap[{lat, lon}] = value;
				else if (param == "GUST") gustMap[{lat, lon}] = value;
			}
		}

		// Zapisywanie danych o wietrze do struktury grid
		for (const auto& coord : uniqueCoords) {
			float lat = coord.first;
			float lon = coord.second;
			grid.latitudes.push_back(lat);
			grid.longitudes.push_back(lon);

			float u = uMap.count(coord) ? uMap[coord] : 0.0f;
			float v = vMap.count(coord) ? vMap[coord] : 0.0f;
			float gust = gustMap.count(coord) ? gustMap[coord] : 0.0f;

			float angle = static_cast<float>(calculateWindAngle(u, v));
			grid.windAngles.push_back(angle);
			grid.windSpeeds.push_back(gust);
		}

		grid.numTiles = uniqueCoords.size();

		std::cout << "Zapisywanie siatki do pliku: " << cacheFileName << std::endl;
		std::ofstream outFile(cacheFileName, std::ios::binary);
		if (outFile.is_open()) {
			outFile.write(reinterpret_cast<const char*>(&grid.numTiles), sizeof(grid.numTiles));

			auto write_vector = [&](const std::vector<float>& vec) {
				size_t size = vec.size();
				outFile.write(reinterpret_cast<const char*>(&size), sizeof(size));
				outFile.write(reinterpret_cast<const char*>(vec.data()), size * sizeof(float));
				};

			write_vector(grid.latitudes);
			write_vector(grid.longitudes);
			write_vector(grid.windAngles);
			write_vector(grid.windSpeeds);

			outFile.close();
			std::cout << "Zakonczono zapisywanie siatki do pliku." << std::endl;
		}
		else {
			std::cerr << "Nie mozna otworzyc pliku do zapisu: " << cacheFileName << std::endl;
		}
	}
	catch (const nlohmann::json::parse_error& e) {
		std::cerr << "Blad parsowania JSON w createGrid: " << e.what() << std::endl;
	}
	std::cout << "Zakonczono tworzenie siatki" << std::endl;

	return grid;
}

void updateOverlayMesh() {
	if (grid.numTiles == 0) return;

	struct Vertex {
		glm::vec3 pos;
		float windSpeed;
	};

	int lonSegments = 360;
	int latSegments = 180;
	std::vector<Vertex> vertices((lonSegments + 1) * (latSegments + 1));

	// Inicjalizacja siatki wierzchołków
	for (int j = 0; j <= latSegments; ++j) {
		for (int i = 0; i <= lonSegments; ++i) {
			float lat = -90.0f + (float)j * (180.0f / latSegments);
			float lon = -180.0f + (float)i * (360.0f / lonSegments);
			int index = j * (lonSegments + 1) + i;
			vertices[index].pos = latLonToXYZ(lat, lon) * modelRadius;
			vertices[index].windSpeed = -1.0f; // Wartość oznaczająca brak danych
		}
	}

	// Wypełnienie siatki danymi z grida
	for (size_t i = 0; i < grid.numTiles; ++i) {
		int lon_idx = static_cast<int>(round(grid.longitudes[i] + 180.0f));
		int lat_idx = static_cast<int>(round(grid.latitudes[i] + 90.0f));

		int index = lat_idx * (lonSegments + 1) + lon_idx;
		if (index >= 0 && index < vertices.size()) {
			vertices[index].windSpeed = grid.windSpeeds[i];
		}
	}

	// Interpolacja brakujących danych
	for (int j = 0; j <= latSegments; ++j) {
		for (int i = 0; i <= lonSegments; ++i) {
			int index = j * (lonSegments + 1) + i;
			if (vertices[index].windSpeed < 0.0f) {
				// Prosta interpolacja liniowa z sąsiadów
				float totalSpeed = 0;
				int count = 0;
				if (i > 0 && vertices[index - 1].windSpeed >= 0) { totalSpeed += vertices[index - 1].windSpeed; count++; }
				if (i < lonSegments && vertices[index + 1].windSpeed >= 0) { totalSpeed += vertices[index + 1].windSpeed; count++; }
				if (j > 0 && vertices[index - (lonSegments + 1)].windSpeed >= 0) { totalSpeed += vertices[index - (lonSegments + 1)].windSpeed; count++; }
				if (j < latSegments && vertices[index + (lonSegments + 1)].windSpeed >= 0) { totalSpeed += vertices[index + (lonSegments + 1)].windSpeed; count++; }

				if (count > 0) {
					vertices[index].windSpeed = totalSpeed / count;
				}
				else {
					vertices[index].windSpeed = 0; // Domyślna wartość, jeśli brak sąsiadów
				}
			}
		}
	}

	// Generowanie indeksów dla trójkątów
	overlayIndices.clear();
	for (int j = 0; j < latSegments; ++j) {
		for (int i = 0; i < lonSegments; ++i) {
			int i1 = j * (lonSegments + 1) + i;
			int i2 = i1 + 1;
			int i3 = (j + 1) * (lonSegments + 1) + i;
			int i4 = i3 + 1;

			if (vertices[i1].windSpeed >= 0.f && vertices[i2].windSpeed >= 0.f && vertices[i3].windSpeed >= 0.f) {
				overlayIndices.push_back(i1);
				overlayIndices.push_back(i3);
				overlayIndices.push_back(i2);
			}
			if (vertices[i2].windSpeed >= 0.f && vertices[i3].windSpeed >= 0.f && vertices[i4].windSpeed >= 0.f) {
				overlayIndices.push_back(i2);
				overlayIndices.push_back(i3);
				overlayIndices.push_back(i4);
			}
		}
	}


	glBindVertexArray(overlayVAO);
	glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, overlayEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, overlayIndices.size() * sizeof(unsigned int), overlayIndices.data(), GL_STATIC_DRAW);

	// Pozycja
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
	// Prędkość wiatru
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, windSpeed));

	glBindVertexArray(0);
}
////////////////////////////////////////////////////

void updateWindDataGlobal() {
	// Aktualizacja globalnej zmiennej windData
	std::cout << "Ladowanie danych o wietrze" << std::endl;
	windDataGlobal = GetWindDataGlobal(date);

	// Tworzenie siatki na podstawie danych o wietrze
	grid = createGrid();

	// Znajdowanie maksymalnej prędkości wiatru i zapisywanie jej w zmiennej globalnej
	if (!grid.windSpeeds.empty()) {
		maxWindSpeed = *std::max_element(grid.windSpeeds.begin(), grid.windSpeeds.end());
		max_speed_str = std::to_string(static_cast<int>(maxWindSpeed));
	}
	else {
		maxWindSpeed = 32.6f; // Domyślna wartość, jeśli brak danych
		max_speed_str = std::to_string(static_cast<int>(maxWindSpeed));
	}

	updateOverlayMesh();
}


///////// Główna funkcja renderująca /////////
void renderScene(GLFWwindow* window)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	float time = glfwGetTime();

	// Rysowanie skyboxa
	drawSkybox();

	// Rysowanie planety
	glm::mat4 rotatedModel = planetModelMatrix * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 1, 0));
	if (show_earthmap) drawObjectTexture(sphereContext, rotatedModel, texture::earth, texture::earthNormal);
	else drawObjectColor(sphereContext, rotatedModel, glm::vec3(0.12f, 0.12f, 0.12f));

	// Rysowanie atmosfery
	glm::mat4 atmosphereModelMatrix = rotatedModel * glm::scale(glm::vec3(1.009f));
	drawObjectAtmosphere(atmosphereContext, atmosphereModelMatrix);

	// Rysowanie granic państw
	glm::mat4 PerspectivexCamera = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 bordersTransform = PerspectivexCamera * planetModelMatrix * glm::scale(glm::vec3(110.0f)); //reverse the earth scaling

	glUseProgram(program);
	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, &bordersTransform[0][0]); // set shaders
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, &planetModelMatrix[0][0]);

	// set values in shaders
	glUniform3f(glGetUniformLocation(program, "color"), 0.0f, 0.0f, 2.55f);
	glUniform3f(glGetUniformLocation(program, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	// draw the actual lines
	glLineWidth(2.0f);
	glBindVertexArray(bordersVAO);
	int counter = 0;
	for (const auto& country : countries) {
		for (size_t i = 0; i < country.boundaries.size(); ++i) {
			if (country.id == selectedCountryId) {
				glLineWidth(10.0f);
			}
			glUniform3f(glGetUniformLocation(program, "color"), borderColor.r, borderColor.g, borderColor.b);
			glDrawArrays(GL_LINE_LOOP, countryFirstVert[counter], countryVertCount[counter]);
			counter++;
			glLineWidth(2.0f);
		}
	}
	glBindVertexArray(0);
	glUseProgram(0);

	if (show_overlay) drawWindOverlay();
	if (show_wind_arrows) drawWindArrows();
	if (selectedCountryId >= 0) {
		CountryWindInfo info = calculateCountryWindInfo(selectedCountryId);
		drawArrow(info.avgLat, info.avgLon, info.avgAngle, glm::vec3(1.0f, 1.0f, 1.0f));
	}
}
////////////////////////////////////////////////////

///////// Funkcje do obsługi okna i modeli /////////
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	aspectRatio = width / float(height);
	glViewport(0, 0, width, height);
}

void loadModelToContext(std::string path, Core::RenderContext& context)
{
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
		return;
	}
	if (scene->mNumMeshes > 0) {
		context.initFromAssimpMesh(scene->mMeshes[0]);
	}
	else {
		std::cout << "ERROR::ASSIMP:: No meshes found in file: " << path << std::endl;
	}
}
////////////////////////////////////////////////////

bool pointInCountry(float testLat, float testLon, const std::vector<glm::vec3>& boundary) {
	if (boundary.size() < 3) return false;

	int intersections = 0;
	for (size_t i = 0, j = boundary.size() - 1; i < boundary.size(); j = i++) {
		float lat1 = glm::degrees(asin(boundary[i].y));
		float lon1 = glm::degrees(atan2(-boundary[i].z, boundary[i].x));
		float lat2 = glm::degrees(asin(boundary[j].y));
		float lon2 = glm::degrees(atan2(-boundary[j].z, boundary[j].x));

		if (((lat1 > testLat) != (lat2 > testLat)) &&
			(testLon < (lon2 - lon1) * (testLat - lat1) / (lat2 - lat1 + 1e-6f) + lon1))
			intersections++;
	}

	return (intersections % 2) == 1;
}
CountryWindInfo calculateCountryWindInfo(int countryId) {
	CountryWindInfo info;

	Country& country = countries[countryId];

	double sumLat = 0.0, sumLon = 0.0;
	int totalPoints = 0;

	for (const auto& boundary : country.boundaries) {
		for (const auto& vertex : boundary) {
			sumLat += glm::degrees(asin(vertex.y));
			sumLon += glm::degrees(atan2(-vertex.z, vertex.x));
			totalPoints++;
		}
	}

	info.avgLat = sumLat / totalPoints;
	info.avgLon = sumLon / totalPoints;

	float maxDistance = gridTileSize * 0.75f;
	double sumSpeed = 0.0, sumX = 0.0, sumY = 0.0;
	int count = 0;

	for (size_t i = 0; i < grid.numTiles; i++) {
		float dLat = grid.latitudes[i] - info.avgLat;
		float dLon = grid.longitudes[i] - info.avgLon;

		if (fabs(dLat) > maxDistance || fabs(dLon) > maxDistance) {
			continue;
		}

		sumSpeed += grid.windSpeeds[i];
		float angleRad = glm::radians(grid.windAngles[i]);
		sumX += cos(angleRad);
		sumY += sin(angleRad);
		count++;
	}

	info.avgSpeed = sumSpeed / count;
	info.avgAngle = glm::degrees(atan2(sumY, sumX));

	if (info.avgAngle < 0.0f) {
		info.avgAngle += 360.0f;
	}
	return info;
}

///////// Funkcja inicjalizująca /////////
void init(GLFWwindow* window)
{
	int width, height, channels;
	unsigned char* image = SOIL_load_image("img/app-icon.png", &width, &height, &channels, SOIL_LOAD_RGBA);

	if (image) {
		GLFWimage icon;
		icon.width = width;
		icon.height = height;
		icon.pixels = image;

		glfwSetWindowIcon(window, 1, &icon);

		SOIL_free_image_data(image);
	}

	// loading borders
	loadCountryBoundaries("data/ne_10m_admin_0_countries/ne_10m_admin_0_countries.shp");

	loadModelToContext("models/sphere2.obj", atmosphereContext);

	std::vector<glm::vec3> vertices;
	countryFirstVert.clear();
	countryVertCount.clear();

	for (const auto& country : countries) {
		for (const auto& boundary : country.boundaries) {
			countryFirstVert.push_back(static_cast<GLsizei>(vertices.size()));
			countryVertCount.push_back(static_cast<GLsizei>(boundary.size()));
			vertices.insert(vertices.end(), boundary.begin(), boundary.end());
		}
	}

	glGenVertexArrays(1, &bordersVAO);
	glGenBuffers(1, &bordersVBO);

	glBindVertexArray(bordersVAO);
	glBindBuffer(GL_ARRAY_BUFFER, bordersVBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glBindVertexArray(0);

	glfwSetMouseButtonCallback(window, [](GLFWwindow* w, int button, int action, int mods) {
		ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplGlfw_MouseButtonCallback(w, button, action, mods);

		if (button != GLFW_MOUSE_BUTTON_LEFT) {
			return;
		}

		if (action == GLFW_PRESS && !io.WantCaptureMouse) {
			int wWidth, wHeight;
			glfwGetFramebufferSize(w, &wWidth, &wHeight);

			double x, y;
			glfwGetCursorPos(w, &x, &y);

			glm::vec3 dir = getRayFromMouse(x, y, wWidth, wHeight);
			glm::vec3 L = -cameraPos;
			float tca = glm::dot(L, dir);
			float d2 = glm::dot(L, L) - tca * tca;
			if (d2 <= planetRadius * planetRadius) {
				glm::vec3 hit = cameraPos + (tca - sqrt(planetRadius * planetRadius - d2)) * dir;
				float lat = glm::degrees(asin(hit.y / planetRadius));
				float lon = glm::degrees(atan2(-hit.z, hit.x));

				selectedCountryId = -1;
				for (const auto& c : countries) {
					for (const auto& b : c.boundaries) {
						if (pointInCountry(lat, lon, b)) {
							selectedCountryId = c.id;
							CountryWindInfo info = calculateCountryWindInfo(selectedCountryId);
							std::cout << "lat: " << info.avgLat << "\n";
							std::cout << "lon: " << info.avgLon << "\n";
							std::cout << "angle: " << info.avgAngle << "\n";
							std::cout << "speed: " << info.avgSpeed << "\n";
							break;
						}
					}
					if (selectedCountryId >= 0) break;
				}
			}

			dragging = true;
			glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwGetCursorPos(w, &lastX, &lastY);
		}
		else if (action == GLFW_RELEASE) {
			dragging = false;
			glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		});



	glfwSetCursorPosCallback(window, [](GLFWwindow* w, double xpos, double ypos) {
		ImGuiIO& io = ImGui::GetIO();
		if (!dragging || io.WantCaptureMouse) {
			return;
		}
		double dx = xpos - lastX;
		double dy = ypos - lastY;
		lastX = xpos;
		lastY = ypos;

		if (abs(dx) > 50 || abs(dy) > 50) {
			return;
		}
		cameraAngleX += dx * mouseSensitivity;
		cameraAngleY += dy * mouseSensitivity;

		float maxAngleY = glm::radians(89.0f);
		cameraAngleY = glm::clamp(cameraAngleY, -maxAngleY, maxAngleY);
		});

	glfwSetScrollCallback(window, scroll_callback);


	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glEnable(GL_DEPTH_TEST);
	
	// Shader atmosfery
	programAtm = shaderLoader.CreateProgram("shaders/shader_5_1_atm.vert", "shaders/shader_5_1_atm.frag");
	if (programAtm == 0) {
		std::cerr << "Blad ladowania shaderow atmosfery!" << std::endl;
		exit(1);
	}

	// Podstawowy shader
	program = shaderLoader.CreateProgram("shaders/shader_5_1.vert", "shaders/shader_5_1.frag");
	if (program == 0) {
		std::cerr << "Blad ladowania podstawowych shaderow!" << std::endl;
		exit(1);
	}

	// Shader modeli z teksturą
	programTex = shaderLoader.CreateProgram("shaders/shader_5_1_tex.vert", "shaders/shader_5_1_tex.frag");
	if (programTex == 0) {
		std::cerr << "Blad ladowania shaderow tekstur!" << std::endl;
		exit(1);
	}

	// Shader nakładki prędkości wiatru
	programOverlay = shaderLoader.CreateProgram("shaders/shader_overlay.vert", "shaders/shader_overlay.frag");
	if (programOverlay == 0) {
		std::cerr << "Blad ladowania shaderow nakladki!" << std::endl;
		exit(1);
	}

	// Shader skyboxa
	programSkybox = shaderLoader.CreateProgram("shaders/shader_skybox.vert", "shaders/shader_skybox.frag");
	if (programSkybox == 0) {
		std::cerr << "Blad ladowania shaderow skyboxa!" << std::endl;
		exit(1);
	}

	// Tekstura Ziemi
	std::cout << "Ladowanie tekstury Ziemi..." << std::endl;
	texture::earth = Core::LoadTexture("textures/earth_smaller.jpg");
	if (texture::earth == 0) {
		std::cerr << "Blad ladowania tekstury Ziemi!" << std::endl;
	}


	// Mapa normalnych ziemi
	std::cout << "Ladowanie mapy normalnych Ziemi..." << std::endl;
	texture::earthNormal = Core::LoadTexture("textures/8k_earth_normal_map.jpg");
	if (texture::earthNormal == 0) { std::cerr << "Blad ladowania mapy normalnych Ziemi!" << std::endl; }

	// Tekstura skyboxa
	std::cout << "Ladowanie tekstury skyboxa..." << std::endl;
	texture::skybox = Core::LoadTexture("textures/skybox.jpg");
	if (texture::skybox == 0) {
		std::cerr << "Blad ladowania tekstury skyboxa!" << std::endl;
	}

	// Model kuli dla skyboxa
	std::cout << "Ladowanie modelu kuli dla skyboxa..." << std::endl;
	loadModelToContext("./models/sphere2.obj", skyboxContext);

	// Model kuli (Ziemi)
	std::cout << "Ladowanie modelu kuli..." << std::endl;
	loadModelToContext("./models/sphere2.obj", sphereContext);

	
	// Model strzałki
	loadModelToContext("./models/arrow.obj", arrowContext);
	std::cout << "Inicjalizacja zakonczona." << std::endl;

	// Shader linii wiatru
	programWindLines = shaderLoader.CreateProgram("shaders/shader_wind_lines.vert", "shaders/shader_wind_lines.frag");
	if (programWindLines == 0) {
		std::cerr << "Blad ladowania shaderow linii wiatru!" << std::endl;
		exit(1);
	}

	///////// Inicjalizacja linii wiatru i nakładki /////////

	std::vector<glm::vec3> lineVertices = {
		{0.0f, -0.5f, -0.5f}, {1.0f, -0.5f, -0.5f}, {1.0f,  0.5f, -0.5f}, {0.0f,  0.5f, -0.5f},
		{0.0f, -0.5f,  0.5f}, {1.0f, -0.5f,  0.5f}, {1.0f,  0.5f,  0.5f}, {0.0f,  0.5f,  0.5f}
	};

	std::vector<unsigned int> lineIndices = {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4,
		0, 1, 5, 5, 4, 0,
		2, 3, 7, 7, 6, 2,
		0, 3, 7, 7, 4, 0, 
		1, 2, 6, 6, 5, 1
	};

	// Utwórz VAO/VBO dla linii
	glGenVertexArrays(1, &windLinesVAO);
	glGenBuffers(1, &windLinesVBO);
	glGenBuffers(1, &windLinesInstanceVBO);

	GLuint lineEBO;
	glGenBuffers(1, &lineEBO);

	glBindVertexArray(windLinesVAO);

	// Wierzchołki linii
	glBindBuffer(GL_ARRAY_BUFFER, windLinesVBO);
	glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(glm::vec3), lineVertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	// Indeksy
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, lineIndices.size() * sizeof(unsigned int), lineIndices.data(), GL_STATIC_DRAW);

	// Instancjonowane macierze transformacji
	glBindBuffer(GL_ARRAY_BUFFER, windLinesInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

	// Ustawienie wskaźników atrybutów dla macierzy modelu
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);

	glBindVertexArray(0);

	// Inicjalizacja VAO/VBO/EBO dla nakładki
	glGenVertexArrays(1, &overlayVAO);
	glGenBuffers(1, &overlayVBO);
	glGenBuffers(1, &overlayEBO);

	//////////////////////////////////////////////////////



	updateWindDataGlobal();
	updateWindArrowData();
}

///////// Czyszczenie po zamknięciu /////////
void shutdown(GLFWwindow* window) {
	shaderLoader.DeleteProgram(program);
	shaderLoader.DeleteProgram(programTex);
	shaderLoader.DeleteProgram(programAtm);
	shaderLoader.DeleteProgram(programOverlay);
	shaderLoader.DeleteProgram(programWindLines);
	shaderLoader.DeleteProgram(programSkybox);
	glDeleteVertexArrays(1, &windLinesVAO);
	glDeleteBuffers(1, &windLinesVBO);
	glDeleteBuffers(1, &windLinesInstanceVBO);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glDeleteTextures(1, &texture::earth);
	glDeleteTextures(1, &texture::earthNormal);
	glDeleteTextures(1, &texture::skybox);
	glDeleteTextures(1, &clock_icon);
	glDeleteTextures(1, &move_icon);
	glDeleteTextures(1, &overlay_icon);
	glDeleteTextures(1, &tutorial_icon);
	glDeleteTextures(1, &date_icon);
	glDeleteTextures(1, &windarrows_icon);

	glDeleteVertexArrays(1, &overlayVAO);
	glDeleteBuffers(1, &overlayVBO);
	glDeleteBuffers(1, &overlayEBO);
}
////////////////////////////////////////////////////

///////// Funkcja do przetwarzania wejścia /////////
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraAngleX += angleSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraAngleX -= angleSpeed;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraAngleY += angleSpeed;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraAngleY -= angleSpeed;

	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		cameraDistance -= moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		cameraDistance += moveSpeed;

	cameraDistance = glm::clamp(cameraDistance, 4.0f, 8.0f);
}


////////////////////////////////////////////////////

///////// Pętla renderująca /////////
void renderLoop(GLFWwindow* window) {
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		// Uruchomienie ImGui
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImVec2 viewport_pos = ImGui::GetMainViewport()->Pos;
		ImVec2 viewport_size = ImGui::GetMainViewport()->Size;

		///////// Floating Button do otwierania QuickMenu /////////

		ImVec2 button_pos = ImVec2(viewport_pos.x + 10, viewport_pos.y + viewport_size.y - 64);
		ImGui::SetNextWindowPos(button_pos, ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.0f);

		ImGuiWindowFlags button_flags = ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNav;

		if (!isQuickMenuOpen) {
			ImGui::Begin("##floating_button", nullptr, button_flags);

			// Zapamiętujemy oryginalne style, aby przywrócić później
			ImVec4 old_bg = ImGui::GetStyleColorVec4(ImGuiCol_Button);
			ImVec4 old_bg_hovered = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
			ImVec4 old_bg_active = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
			float old_rounding = ImGui::GetStyle().FrameRounding;

			// Ustawiamy styl dla przycisku
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.71f, 0.847f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.79f, 0.89f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.50f, 0.60f, 1.0f));

			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 50.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

			if (ImGui::Button(u8"Menu", ImVec2(100, 0))) isQuickMenuOpen = true;

			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(3);

			ImGui::End();
		}
		////////////////////////////////////////////////////

		///////// Quick Menu /////////
		std::string dateText = date.substr(6, 2) + "." + date.substr(4, 2) + "." + date.substr(0, 4) + " 0:00 UTC";

		ImVec2 window_pos = ImVec2(viewport_pos.x, viewport_pos.y + viewport_size.y - ImGuiHeight);

		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(ImGuiWidth, ImGuiHeight), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.85f);
		if (isQuickMenuOpen && ImGui::Begin("title###Imguilayout", &isQuickMenuOpen, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar))
		{
			if (!clock_icon)
				clock_icon = Core::LoadTexture("img/time-outline.png");
			ImGui::Image((ImTextureID)(intptr_t)clock_icon, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1)); //StretchPolicy::Scale

			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			ImGui::TextUnformatted(u8"Prędkość animacji");

			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			ImGui::SetNextItemWidth(200);
			if (ImGui::SliderInt("##animationSpeed", &animationSpeed, 10, 1000, nullptr))
			{
				animationSpeed = animationSpeed;
			};

			if (!move_icon)
				move_icon = Core::LoadTexture("img/move-outline.png");
			ImGui::Image((ImTextureID)(intptr_t)move_icon, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1)); 

			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			ImGui::TextUnformatted(u8"Prędkość ruchu kamery");

			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			ImGui::SetNextItemWidth(200);
			if (ImGui::SliderInt("##cameraSpeed", &cameraSpeed, 10, 1000, nullptr)) {
				angleSpeed = 0.01f * (cameraSpeed / 100.0f);
				moveSpeed = 0.01f * (cameraSpeed / 100.0f);
				rotationSpeed = 0.01f * (cameraSpeed / 100.0f);
			}

			if (!overlay_icon)
				overlay_icon = Core::LoadTexture("img/layers-outline.png");
			ImGui::Image((ImTextureID)(intptr_t)overlay_icon, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1));

			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			ImGui::TextUnformatted(u8"Nakładka prędkości wiatru");

			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			ImGui::Checkbox("##show_overlay", &show_overlay);
			if (!earthmap_icon)
				earthmap_icon = Core::LoadTexture("img/earth-outline.png");
			ImGui::Image((ImTextureID)(intptr_t)earthmap_icon, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1));

			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			ImGui::TextUnformatted(u8"Mapa satelitarna");

			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			ImGui::Checkbox("##show_earthmap", &show_earthmap);
			if (!tutorial_icon)
				tutorial_icon = Core::LoadTexture("img/book-outline.png");
			ImGui::Image((ImTextureID)(intptr_t)tutorial_icon, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1));

			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			ImGui::TextUnformatted(u8"Samouczek");

			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			ImGui::Checkbox("##show_tutorial", &show_tutorial);
			
			if (!windarrows_icon)
				windarrows_icon = Core::LoadTexture("img/wind-outline.png");
			ImGui::Image((ImTextureID)(intptr_t)windarrows_icon, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1));
			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			ImGui::TextUnformatted(u8"Linie wiatru");

			ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x);
			ImGui::Checkbox("##show_wind_arrows", &show_wind_arrows);


			if (!date_icon)
				date_icon = Core::LoadTexture("img/calendar-outline.png");
			ImGui::Image((ImTextureID)(intptr_t)date_icon, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1)); 

			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			ImGui::TextUnformatted(u8"Data: ");

			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			ImGui::PushStyleColor(ImGuiCol_Text, 0xffffcc00);
			ImGui::TextUnformatted(dateText.c_str());
			ImGui::PopStyleColor();

			if (ImGui::Button(u8"-1 Dzień", ImVec2(100, 40)))
			{
				if (daysBefore + 1 >= 0 && daysBefore + 1 <= 7) {
					daysBefore += 1;
					date = GetFormattedDate(-daysBefore);
					dateText = date.substr(6, 2) + "." + date.substr(4, 2) + "." + date.substr(0, 4) + " 0:00 UTC";;
				}
			}

			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			if (ImGui::Button(u8"+1 Dzień", ImVec2(100, 40)))
			{
				if (daysBefore - 1 >= 0 && daysBefore - 1 <= 7) {
					daysBefore -= 1;
					date = GetFormattedDate(-daysBefore);
					dateText = date.substr(6, 2) + "." + date.substr(4, 2) + "." + date.substr(0, 4) + " 0:00 UTC";;
				}
			}

			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			if (ImGui::Button(u8"Dzisiaj", ImVec2(100, 40))) {
				daysBefore = 0;
				date = GetFormattedDate(-daysBefore);
				dateText = date.substr(6, 2) + "." + date.substr(4, 2) + "." + date.substr(0, 4) + " 0:00 UTC";
			}

			if (ImGui::Button(u8"Zmień datę", ImVec2(320, 40)) && !isUpdating)
			{
				show_tutorial = false;
				updateWindDataGlobal();
				updateWindArrowData();
				isUpdating = false;
				
			}

			if (ImGui::Button(u8"Zamknij", ImVec2(168, 40)))
			{
				isQuickMenuOpen = false;
			}

			ImGui::End();
		}
		////////////////////////////////////////////////////

		///////// Legenda Mapy /////////
		ImGuiWindowFlags legend_flags = ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNav;

		const float padding = 10.0f;
		ImVec2 legend_pos = ImVec2(viewport_pos.x + viewport_size.x - padding,
			viewport_pos.y + viewport_size.y - padding);
		ImVec2 legend_pivot = ImVec2(1.0f, 1.0f);
		ImGui::SetNextWindowPos(legend_pos, ImGuiCond_Always, legend_pivot);
		ImGui::SetNextWindowBgAlpha(0.75f);
		ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_Always);
		if (ImGui::Begin("Legenda", nullptr, legend_flags)) {
			ImGui::Text(u8"Prędkość wiatru (m/s)");
			ImGui::Separator();

			float w = 284.0f;
			float h = 20.0f;
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			ImVec2 p = ImGui::GetCursorScreenPos();
			ImU32 col_blue = ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
			ImU32 col_green = ImGui::GetColorU32(ImVec4(0.0f, 1.0f, 0.0f, 1.0f));

			draw_list->AddRectFilledMultiColor(p, ImVec2(p.x + w, p.y + h), col_blue, col_green, col_green, col_blue);
			ImGui::Dummy(ImVec2(w, h));

			ImGui::Text("0.0");
			ImGui::SameLine();

			
			float text_width = ImGui::CalcTextSize(max_speed_str.c_str()).x;
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - text_width);
			ImGui::Text("%s", max_speed_str.c_str());
		}
		ImGui::End();
		////////////////////////////////////////////////////

		// Render sceny
		renderScene(window);

		// Samouczek
		if (show_tutorial && !tutorial_popup_open) {
			ImGui::OpenPopup("Tutorial");
			tutorial_popup_open = true;
			tutorial_step = -1;
		}

		// wstęp
		if (tutorial_step == -1) {
			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImVec2 size(400, 200);
			ImGui::SetNextWindowSize(size, ImGuiCond_Always);
			ImGui::SetNextWindowPos(ImVec2(center.x - size.x * 0.5f, center.y - size.y * 0.5f), ImGuiCond_Always);

			if (ImGui::BeginPopupModal("Tutorial", nullptr, ImGuiWindowFlags_NoResize)) {
				ImGui::TextWrapped(u8"Witaj w projekcie wizualizującym ruchy wiatrów na Ziemi.");
				ImGui::Spacing();
				ImGui::TextWrapped(u8"Podczas samouczka zobaczysz prezentowane efekty w tle.");
				ImGui::Spacing();
				if (ImGui::Button(u8"Start", ImVec2(120, 40))) {
					show_overlay = false;
					show_wind_arrows = false;
					show_earthmap = false;
					selectedCountryId = -1;
					tutorial_step = 0;
					zoomCamera = true;
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}

		// tutorial
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

		ImVec2 pos = ImGui::GetMainViewport()->WorkPos;

		ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.85f);

		if (ImGui::Begin("TutorialStep", nullptr, flags)) {
			if (show_tutorial) cameraAngleX -= rotationSpeed;
			if (tutorial_step == 0 && show_tutorial) {
				tutorial_steps_initialized[1] = false;
				tutorial_steps_initialized[2] = false;
				tutorial_steps_initialized[3] = false;
					if (!tutorial_steps_initialized[0]) {
					show_earthmap = true;
					show_overlay = false;
					show_wind_arrows = false;
					isQuickMenuOpen = true;
					tutorial_steps_initialized[0] = true;
				}
				ImGui::TextWrapped(u8"Krok 1: Mapa satelitarna");
				ImGui::BulletText(u8"W menu możesz włączać i wyłączać teksturę planety.");
				if (ImGui::Button(u8"Dalej", ImVec2(100, 36))) {
					tutorial_step = 1;
				}
			}
			else if (tutorial_step == 1 && show_tutorial) {
				tutorial_steps_initialized[0] = false;
				tutorial_steps_initialized[2] = false;
				tutorial_steps_initialized[3] = false;
				if (!tutorial_steps_initialized[1]) {
					show_earthmap = true;
					show_overlay = true;
					show_wind_arrows = false;
					isQuickMenuOpen = true;
					tutorial_steps_initialized[1] = true;
				}
				ImGui::TextWrapped(u8"Krok 2: Nakładka prędkości wiatru");
				ImGui::BulletText(u8"W menu możesz włączać i wyłączać nakładkę wiatru.");
				ImGui::BulletText(u8"Mapuje ona prędkość wiatru kolorystycznie, według legendy w prawym dolnym rogu aplikacji.");
				if (ImGui::Button(u8"Dalej", ImVec2(100, 36))) {
					tutorial_step = 2;
				}
			}
			else if (tutorial_step == 2 && show_tutorial) {
				tutorial_steps_initialized[0] = false;
				tutorial_steps_initialized[1] = false;
				tutorial_steps_initialized[3] = false;
				if (!tutorial_steps_initialized[2]) {
					show_earthmap = true;
					show_overlay = true;
					show_wind_arrows = true;
					isQuickMenuOpen = true;
					tutorial_steps_initialized[2] = true;
				}
				ImGui::TextWrapped(u8"Krok 3: Linie wiatru");
				ImGui::BulletText(u8"Animowane linie pokazują kierunki i siłę wiatru.");
				ImGui::BulletText(u8"Również możesz je włączać i wyłączać w menu.");
				if (ImGui::Button(u8"Dalej", ImVec2(100, 36))) {
					tutorial_step = 3;
				}
			}
			else if (tutorial_step == 3 && show_tutorial) {
				tutorial_steps_initialized[0] = false;
				tutorial_steps_initialized[1] = false;
				tutorial_steps_initialized[2] = false;
				if (!tutorial_steps_initialized[3]) {
					show_earthmap = true;
					show_overlay = true;
					show_wind_arrows = true;
					isQuickMenuOpen = true;
					tutorial_steps_initialized[3] = true;
				}
				ImGui::TextWrapped(u8"Krok 4: Odczytywanie informacji o wietrze");
				ImGui::BulletText(u8"Każda linia przedstawia kierunek i siłę wiatru w danym punkcie.");
				ImGui::BulletText(u8"Długość linii oraz prędkość animacji zależą od prędkości wiatru.");
				ImGui::BulletText(u8"Kierunek linii i kierunek animacji wizualizują kierunek wiatru.");
				if (ImGui::Button(u8"Dalej", ImVec2(100, 36))) {
					tutorial_step = 4;
				}
			}
			else if (tutorial_step == 4 && show_tutorial) {
				tutorial_steps_initialized[0] = false;
				tutorial_steps_initialized[1] = false;
				tutorial_steps_initialized[2] = false;
				tutorial_steps_initialized[3] = false;
				ImGui::TextWrapped(u8"Krok 5: Sterowanie i zaznaczanie");
				ImGui::BulletText(u8"Możesz użyć klawiszy W, S, A, D do poruszania kamerą.");
				ImGui::BulletText(u8"Klawisze Q, E służą do przybliżania i oddalania kamery.");
				ImGui::BulletText(u8"Po kliknięciu LPM na wybrany kraj, zobaczysz dominujący kierunek wiatru.");
				if (ImGui::Button(u8"Dalej", ImVec2(100, 36))) {
					tutorial_step = 5;
				}
			}
			else if (tutorial_step == 5 && show_tutorial) {
				tutorial_steps_initialized[0] = false;
				tutorial_steps_initialized[1] = false;
				tutorial_steps_initialized[2] = false;
				tutorial_steps_initialized[3] = false;
				ImGui::TextWrapped(u8"To wszystko!");
				ImGui::BulletText(u8"Teraz możesz odkrywać globalne zjawiska pogodowe i analizować wiatry na całym świecie.");
				ImGui::BulletText(u8"W razie potrzeby, zawsze możesz ponownie uruchomić samouczek w menu.");
				if (ImGui::Button(u8"Zakończ", ImVec2(100, 36))) {
					show_tutorial = false;
					tutorial_popup_open = false;
					tutorial_step = -1;
					revertCamera = true;
				}
			}

		}
		ImGui::End();

		if (revertCamera) revertCameraXToDefault();
		if (zoomCamera) zoomCameraToDefault();
		// Render ImGui
		ImGui::Render();

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}
////////////////////////////////////////////////////
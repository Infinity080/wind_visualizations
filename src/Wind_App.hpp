#include "glew.h"
#include <GLFW/glfw3.h>
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>


#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Texture.h"

#include "Box.cpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include "shapefil.h"

#include "Get_Wind_Data.h"
#include "imgui_internal.h"

#include <nlohmann/json.hpp>



///////// Inicjalizacje zmiennych //////////
// Inicjalizacja zmiennych dla tekstur
namespace texture {
	GLuint earth;
	GLuint clouds;
	GLuint earthNormal;
	GLuint cloudsT;
}

// Inicjalizacja zmiennych dla siatki
struct Grid {
	std::vector<float> latitudes;   // Lista szerokości geograficznych środków kafelków
	std::vector<float> longitudes;  // Lista długości geograficznych środków kafelków
	std::vector<float> windAngles; // Lista kierunków wiatru w stopniach
	std::vector<float> windSpeeds; // Lista prędkości wiatru w m/s
	size_t numTiles; // Liczba kafelków w siatce
};

struct Country {
	int id;
	std::string name;
	std::vector<std::vector<glm::vec3>> boundaries;
};

std::vector<Country> countries;

int selectedCountryId = -1;

Grid grid;

float gridTileSize = 11.0f; // Rozmiar pojedynczego kafelka siatki - docelowo 1.0 lub 0.5f


// Inicjalizacja zmiennych dla programow shaderow
GLuint program; // Podstawowy program do rysowania kolorem
GLuint programTex; // Program do rysowania z teksturą
GLuint programAtm; // Program do rysowania atmosfery
GLuint programCloud; // Program do rysowania chmur
GLuint programArrowInstanced; // Program do rysowania strzałek za pomocą instancingu

Core::Shader_Loader shaderLoader;

Core::RenderContext sphereContext;
Core::RenderContext arrowContext;
GLuint arrowInstanceVBO; // VBO dla danych instancji strzałek (macierze modelu)
std::vector<glm::mat4> arrowModelMatrices; // Wektor macierzy modelu dla każdej strzałki

// Zmienne kamery
glm::vec3 cameraPos = glm::vec3(-6.f, 0, 0);
glm::vec3 cameraDir = glm::vec3(1.f, 0.f, 0.f);
float aspectRatio = 1.f;
float angleSpeed = 0.01f;
float moveSpeed = 0.01f;

float cameraAngleX = 0.0f;
float cameraAngleY = 0.0f;
float cameraDistance = 6.0f;

bool dragging = false;
double lastX = 0.0;
double lastY = 0.0;
float mouseSensitivity = 0.005f;

GLuint bordersVAO = 0, bordersVBO = 0;
std::vector<GLuint> countryFirstVert, countryVertCount;

float planetRadius = 3.0f;
float modelRadius = 110.0f;
float pointRadius = 0.00015f;

glm::vec3 planetScale = glm::vec3(planetRadius / modelRadius);
glm::mat4 planetModelMatrix = glm::scale(glm::mat4(1.0f), planetScale);

float pointRadiusModel = pointRadius / planetScale.x;

float arrowSize = 0.005f;
float arrowScaleModel = arrowSize * modelRadius / planetRadius;

GLFWwindow* windowGlobal;

std::string windDataGlobal; // Globalne dane o wietrze


// Zmienne QuickMenu
int cameraSpeed = 100;
int animationSpeed = 100;
bool show_overlay = true;
bool show_tutorial = true;
int daysBefore = 0;
bool isQuickMenuOpen = false;
GLuint clock_icon = 0;
GLuint move_icon = 0;
GLuint overlay_icon = 0;
GLuint tutorial_icon = 0;
GLuint date_icon = 0;
float ImGuiWidth = 460.0f;
float ImGuiHeight = 280.0f;

std::string date = GetFormattedDate(-daysBefore); // Dzisiejsza data

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
	glUniform3f(glGetUniformLocation(prog, "lightPos"), -5.f, 3.f, 3.f);
	glUniform3f(glGetUniformLocation(prog, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	Core::DrawContext(context);
	glUseProgram(0);
}

void drawPoint(float lat, float lon, glm::vec3 color) {
	glm::vec3 dir = latLonToXYZ(lat, lon);

	glm::vec3 pos = dir * modelRadius;

	glm::mat4 model = planetModelMatrix
		* glm::translate(glm::mat4(1.0f), pos) // przesuwamy na powierzchnię
		* glm::scale(glm::mat4(1.0f), glm::vec3(pointRadiusModel)); // zmniejszamy

	drawObjectColor(sphereContext, model, color);
}

void drawPoint2D(float lat, float lon, glm::vec3 color) {
	// Obliczenie pozycji 3D na powierzchni kuli
	glm::vec3 worldPos = latLonToXYZ(lat, lon) * modelRadius;

	// Przygotowanie VAO i VBO dla punktu
	GLuint pointVAO, pointVBO;
	glGenVertexArrays(1, &pointVAO);
	glGenBuffers(1, &pointVBO);

	glBindVertexArray(pointVAO);
	glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), &worldPos, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	// Użycie shadera i ustawienie uniformów
	glUseProgram(program);
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * planetModelMatrix;

	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniform3f(glGetUniformLocation(program, "color"), color.x, color.y, color.z);

	// Ustawienie rozmiaru punktu i jego rysowanie
	glPointSize(5.0f);
	glDrawArrays(GL_POINTS, 0, 1);

	// Czyszczenie
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &pointVAO);
	glDeleteBuffers(1, &pointVBO);
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
	glUniformMatrix4fv(glGetUniformLocation(prog, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(prog, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(prog, "lightPos"), -5.f, 3.f, 3.f);
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
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(prog, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(prog, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(prog, "lightPos"), -5.f, 3.f, 3.f);
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

// Funkcja do rysowania chmur
void drawObjectClouds(Core::RenderContext& context, glm::mat4 modelMatrix, GLuint cloudColorTexture) {
	GLuint prog = programCloud;
	glUseProgram(prog);

	// Włączenie standardowego alpha blendingu
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	glm::mat4 viewProjection = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjection * modelMatrix;

	glUniformMatrix4fv(glGetUniformLocation(prog, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(prog, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(prog, "lightPos"), -5.f, 3.f, 3.f);
	glUniform3f(glGetUniformLocation(prog, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	glUniform1f(glGetUniformLocation(prog, "shininess"), 30.0f);

	Core::SetActiveTexture(cloudColorTexture, "cloudColor", prog, 0);

	Core::DrawContext(context);

	// Wyłączenie blendingu i przywrócenie zapisu głębokosci
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	glUseProgram(0);
}

// Funkcja do aktualizacji danych o strzałkach wiatru dla renderingu instancyjnego
void updateWindArrowData() {
	arrowModelMatrices.clear();
	if (grid.numTiles == 0) {
		return;
	}

	try {
		int timeIndex = 0;

		arrowModelMatrices.reserve(grid.numTiles);

		for (size_t i = 0; i < grid.numTiles; ++i) {
			try {
				float lat = grid.latitudes[i];
				float lon = grid.longitudes[i];
				float arrowAngle = grid.windAngles[i];
				float arrowSpeed = grid.windSpeeds[i];

				glm::vec3 normal = glm::normalize(latLonToXYZ(lat, lon));
				glm::vec3 point = normal * (modelRadius + 1.0f);
				glm::vec3 Y = glm::normalize(glm::cross(glm::vec3(0, 1, 0), normal));
				glm::vec3 X = glm::normalize(glm::cross(normal, Y));
				float alpha = glm::radians(arrowAngle);
				glm::vec3 rot = glm::normalize(X * glm::cos(alpha) + Y * glm::sin(alpha));
				glm::mat4 modelMatrix = planetModelMatrix
					* glm::translate(glm::mat4(1), point)
					* glm::mat4(glm::rotation(glm::vec3(0, 0, 1), rot))
					* glm::scale(glm::mat4(1), glm::vec3(arrowScaleModel));

				arrowModelMatrices.push_back(modelMatrix);
			}
			catch (const std::exception& e) {
				// Jeśli wystąpi błąd podczas przetwarzania konkretnego punktu, pomijamy go
			}
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Błąd podczas przetwarzania danych o wietrze: " << e.what() << std::endl;
		return;
	}

	// Aktualizacja VBO z macierzami modeli
	glBindBuffer(GL_ARRAY_BUFFER, arrowInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER, arrowModelMatrices.size() * sizeof(glm::mat4), arrowModelMatrices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

double calculateWindAngle(double u, double v) {
	const double PI = 3.14159265358979323846;
	double angleRad = atan2(u, v); // Obliczamy kierunek strzałki w radianach
	double angleDeg = angleRad * 180.0 / PI;
	if (angleDeg < 0) angleDeg += 360.0;
	return angleDeg;
}

// Funkcja do rysowania strzałek wiatru
void drawWindArrows() {

	if (arrowModelMatrices.empty()) {
		return;
	}

	GLuint prog = programArrowInstanced;
	glUseProgram(prog);

	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glUniformMatrix4fv(glGetUniformLocation(prog, "viewProjectionMatrix"), 1, GL_FALSE, (float*)&viewProjectionMatrix);
	glUniform3f(glGetUniformLocation(prog, "color"), 1.0f, 1.0f, 1.0f);
	glUniform3f(glGetUniformLocation(prog, "lightPos"), -5.f, 3.f, 3.f);
	glUniform3f(glGetUniformLocation(prog, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	glBindVertexArray(arrowContext.vertexArray);
	glDrawElementsInstanced(GL_TRIANGLES, arrowContext.size, GL_UNSIGNED_INT, 0, arrowModelMatrices.size());
	glBindVertexArray(0);

	glUseProgram(0);
}
////////////////////////////////////////////////////

///////// Funkcje do siatki //////////
// Funkcja do tworzenia siatki składającej się z kafelków
Grid createGrid() {
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
	}
	catch (const nlohmann::json::parse_error& e) {
		std::cerr << "Blad parsowania JSON w createGrid: " << e.what() << std::endl;
	}
	std::cout << "Zakonczono tworzenie siatki" << std::endl;

	return grid;
}

// Funkcja do rysowania punktów (środków) siatki
void drawGridDots() {

	// Kolor punktów
	glm::vec3 pointColor(1.0f, 1.0f, 1.0f);

	// Rysowanie punktów
	for (size_t i = 0; i < grid.numTiles; ++i) {
		drawPoint2D(grid.latitudes[i], grid.longitudes[i], pointColor);
	}

}
////////////////////////////////////////////////////

void updateWindDataGlobal() {
	// Aktualizacja globalnej zmiennej windData
	windDataGlobal = GetWindDataGlobal(date);

	// Tworzenie siatki na podstawie danych o wietrze
	grid = createGrid();
}


///////// Główna funkcja renderująca /////////
void renderScene(GLFWwindow* window)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	float time = glfwGetTime();

	// Rysowanie planety
	drawObjectColor(sphereContext, planetModelMatrix, glm::vec3(1.7f, 1.7f, 2.55f));

	// Drawing boundaries
	glm::mat4 PerspectivexCamera = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 bordersTransform = PerspectivexCamera * planetModelMatrix * glm::scale(glm::vec3(110.0f));//reverse the earth scaling

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
			glm::vec3 color = (country.id == selectedCountryId)
				? glm::vec3(1.0f, 0.2f, 0.2f)
				: glm::vec3(0.0f, 0.0f, 2.55f);

			glUniform3f(glGetUniformLocation(program, "color"), color.r, color.g, color.b);
			glDrawArrays(GL_LINE_LOOP, countryFirstVert[counter], countryVertCount[counter]);
			counter++;
		}
	}
	glBindVertexArray(0);
	glUseProgram(0);

	// granice zaznaczonego kraju
	if (selectedCountryId >= 0) {
		const Country& selected = countries[selectedCountryId];
		for (const auto& boundary : selected.boundaries) {
			for (const auto& v : boundary) {
				float lat = glm::degrees(asin(v.y));
				float lon = glm::degrees(atan2(-v.z, v.x));
				drawPoint2D(lat, lon, glm::vec3(1.0f, 0.0f, 0.0f)); 
			}
		}
	}

	drawWindArrows();
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

///////// Funkcja inicjalizująca /////////
void init(GLFWwindow* window)
{
	// loading borders
	loadCountryBoundaries("data/ne_10m_admin_0_countries/ne_10m_admin_0_countries.shp");

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

	// Shader atmosfery
	programAtm = shaderLoader.CreateProgram("shaders/shader_5_1_atm.vert", "shaders/shader_5_1_atm.frag");
	if (programAtm == 0) {
		std::cerr << "Blad ladowania shaderow atmosfery!" << std::endl;
		exit(1);
	}

	// Shader chmur
	programCloud = shaderLoader.CreateProgram("shaders/shader_5_1_cloud.vert", "shaders/shader_5_1_cloud.frag");
	if (programCloud == 0) {
		std::cerr << "Blad ladowania shaderow chmur!" << std::endl;
		exit(1);
	}

	programArrowInstanced = shaderLoader.CreateProgram("shaders/shader_arrow_instanced.vert", "shaders/shader_5_1.frag");
	if (programArrowInstanced == 0) {
		std::cerr << "Blad ladowania shaderow instancingu strzalek!" << std::endl;
		exit(1);
	}

	// Mapa normalnych ziemi
	std::cout << "Ladowanie mapy normalnych Ziemi..." << std::endl;
	texture::earthNormal = Core::LoadTexture("textures/Mandalore Legends (Bump 4k).png");
	if (texture::earthNormal == 0) { std::cerr << "Blad ladowania mapy normalnych Ziemi!" << std::endl; }

	// Mapa normalnych chmur
	std::cout << "Ladowanie mapy bump/normal chmur..." << std::endl;
	texture::cloudsT = Core::LoadTexture("textures/Taris (Clouds Bump 4k).png");
	if (texture::cloudsT == 0) { std::cerr << "Blad ladowania mapy bump/normal chmur!" << std::endl; }

	// Model kuli (Ziemi)
	std::cout << "Ladowanie modelu kuli..." << std::endl;
	loadModelToContext("./models/sphere2.obj", sphereContext);

	// Tekstura Ziemi
	std::cout << "Ladowanie tekstury Ziemi..." << std::endl;
	texture::earth = Core::LoadTexture("textures/Mandalore Legends (Diffuse 4k).png");
	if (texture::earth == 0) {
		std::cerr << "Blad ladowania tekstury Ziemi!" << std::endl;
	}

	// Tekstura chmur
	std::cout << "Ladowanie tekstury chmur..." << std::endl;
	texture::clouds = Core::LoadTexture("textures/Taris (Clouds 4k).png");
	if (texture::clouds == 0) {
		std::cerr << "Blad ladowania tekstury Chmur!" << std::endl;
	}

	std::cout << "Inicjalizacja zakonczona." << std::endl;
	// loading arrow model
	loadModelToContext("./models/arrow.obj", arrowContext);

	// Konfiguracja renderingu instancyjnego dla strzałek
	glGenBuffers(1, &arrowInstanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, arrowInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW); // Pusta na początku

	glBindVertexArray(arrowContext.vertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, arrowInstanceVBO);

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

	// Globalne dane o wietrze
	std::cout << "Ladowanie globalnych danych o wietrze" << std::endl;

	//updateWindDataGlobal();
	//updateWindArrowData();
}

///////// Czyszczenie po zamknięciu /////////
void shutdown(GLFWwindow* window) {
	shaderLoader.DeleteProgram(program);
	shaderLoader.DeleteProgram(programTex);
	shaderLoader.DeleteProgram(programAtm);
	shaderLoader.DeleteProgram(programCloud);
	shaderLoader.DeleteProgram(programArrowInstanced);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glDeleteTextures(1, &texture::earth);
	glDeleteTextures(1, &texture::clouds);
	glDeleteTextures(1, &texture::earthNormal);
	glDeleteTextures(1, &texture::cloudsT);
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
		windowGlobal = window;
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

		ImGui::Begin("##floating_button", nullptr, button_flags);

		// Zapamietujemy oryginalne style, aby przywrócic po przycisku
		ImVec4 old_bg = ImGui::GetStyleColorVec4(ImGuiCol_Button);
		ImVec4 old_bg_hovered = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
		ImVec4 old_bg_active = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
		float old_rounding = ImGui::GetStyle().FrameRounding;

		// Ustawiamy styl dla okraglego przycisku
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.71f, 0.847f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.79f, 0.89f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.50f, 0.60f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 50.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

		if (ImGui::Button("Menu", ImVec2(100, 0))) isQuickMenuOpen = true;

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);

		ImGui::End();
		////////////////////////////////////////////////////

		///////// Quick Menu /////////
		std::string dateText = date.substr(6, 2) + "." + date.substr(4, 2) + "." + date.substr(0, 4);
	
		ImVec2 window_pos = ImVec2(viewport_pos.x, viewport_pos.y + viewport_size.y - ImGuiHeight);

		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(ImGuiWidth, ImGuiHeight), ImGuiCond_Always);
		if (isQuickMenuOpen && ImGui::Begin("title###Imguilayout", &isQuickMenuOpen, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar))
		{
			/// @separator

			/// @begin Image
			if (!clock_icon)
				clock_icon = Core::LoadTexture("img/time-outline.png");
			ImGui::Image((ImTextureID)(intptr_t)clock_icon, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1)); //StretchPolicy::Scale
			/// @end Image

			/// @begin Text
			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			//ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Predkosc animacji");
			/// @end Text

			/// @begin Slider
			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			ImGui::SetNextItemWidth(200);
			if (ImGui::SliderInt("##animationSpeed", &animationSpeed, 10, 1000, nullptr))
			{
				animationSpeed = animationSpeed;
			};
			/// @end Slider

			/// @begin Image
			if (!move_icon)
				move_icon = Core::LoadTexture("img/move-outline.png");
			ImGui::Image((ImTextureID)(intptr_t)move_icon, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1)); //StretchPolicy::Scale
			/// @end Image

			/// @begin Text
			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			//ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Predkosc ruchu kamery");
			/// @end Text

			/// @begin Slider
			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			ImGui::SetNextItemWidth(200);
			if (ImGui::SliderInt("##cameraSpeed", &cameraSpeed, 10, 1000, nullptr)) {
				angleSpeed = 0.01f * (cameraSpeed / 100.0f);
				moveSpeed = 0.01f * (cameraSpeed / 100.0f);
			}
			/// @end Slider

			/// @begin Image
			if (!overlay_icon)
				overlay_icon = Core::LoadTexture("img/layers-outline.png");
			ImGui::Image((ImTextureID)(intptr_t)overlay_icon, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1)); //StretchPolicy::Scale
			/// @end Image

			/// @begin Text
			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			//ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Nakladka predkosci wiatru");
			/// @end Text

			/// @begin CheckBox
			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			ImGui::Checkbox("##show_overlay", &show_overlay);
			/// @end CheckBox

			/// @begin Image
			if (!tutorial_icon)
				tutorial_icon = Core::LoadTexture("img/book-outline.png");
			ImGui::Image((ImTextureID)(intptr_t)tutorial_icon, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1)); //StretchPolicy::Scale
			/// @end Image

			/// @begin Text
			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			//ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Samouczek");
			/// @end Text

			/// @begin CheckBox
			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			ImGui::Checkbox("##show_tutorial", &show_tutorial);
			/// @end CheckBox

			/// @begin Image
			if (!date_icon)
				date_icon = Core::LoadTexture("img/calendar-outline.png");
			ImGui::Image((ImTextureID)(intptr_t)date_icon, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1)); //StretchPolicy::Scale
			/// @end Image

			/// @begin Text
			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			//ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Data: ");
			/// @end Text

			/// @begin Text
			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			ImGui::PushStyleColor(ImGuiCol_Text, 0xffffcc00);
			//ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(dateText.c_str());
			ImGui::PopStyleColor();
			/// @end Text

			/// @begin Button
			if (ImGui::Button("-1 Dzien", ImVec2(100, 0)))
			{
				if (daysBefore + 1 >= 0 && daysBefore + 1 <= 7) {
					daysBefore += 1;
					date = GetFormattedDate(-daysBefore);
					dateText = date.substr(6, 2) + "." + date.substr(4, 2) + "." + date.substr(0, 4);
				}
			}
			/// @end Button

			/// @begin Button
			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			if (ImGui::Button("+1 Dzien", ImVec2(100, 0)))
			{
				if (daysBefore - 1 >= 0 && daysBefore - 1 <= 7) {
					daysBefore -= 1;
					date = GetFormattedDate(-daysBefore);
					dateText = date.substr(6, 2) + "." + date.substr(4, 2) + "." + date.substr(0, 4);
				}
			}
			/// @end Button

			/// @begin Button
			ImGui::SameLine(0, 1 * ImGui::GetStyle().ItemSpacing.x);
			if (ImGui::Button("Dzisiaj", ImVec2(100, 0))) {
				daysBefore = 0;
				date = GetFormattedDate(-daysBefore);
				dateText = date.substr(6, 2) + "." + date.substr(4, 2) + "." + date.substr(0, 4);
			}
			/// @end Button

			/// @begin Button
			if (ImGui::Button("Zmien date", ImVec2(320, 0)))
			{
				updateWindDataGlobal();
				updateWindArrowData();
			}
			/// @end Button

			/// @begin Button
			if (ImGui::Button("Zamknij", ImVec2(168, 32)))
			{
				isQuickMenuOpen = false;
			}
			/// @end Button

			/// @separator
			ImGui::End();
		}
		/// @end TopWindow

		////////////////////////////////////////////////////


		// Render sceny
		renderScene(window);

		// Render ImGui
		ImGui::Render();

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}
////////////////////////////////////////////////////
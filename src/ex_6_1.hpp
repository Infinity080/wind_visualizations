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

// Przestrzen nazw dla tekstur
namespace texture {
	// Miejsce na deklaracje zmiennych tekstur (Etapy 2, 4, 5)
	GLuint earth;
	GLuint clouds;
	GLuint earthNormal;
	GLuint cloudsT;
}

// Zmienne globalne dla programow shaderow
GLuint program; // Podstawowy program do rysowania kolorem
// Miejsce na deklaracje zmiennych programow shaderow (Etapy 2, 3, 4)
GLuint programTex;
GLuint programAtm;
GLuint programCloud;

Core::Shader_Loader shaderLoader;

Core::RenderContext shipContext;
Core::RenderContext sphereContext;

// Ustawienia kamery i statku
glm::vec3 cameraPos = glm::vec3(-4.f, 0, 0);
glm::vec3 cameraDir = glm::vec3(1.f, 0.f, 0.f);
glm::vec3 spaceshipPos = glm::vec3(0.35f, 0, 5.0f);
glm::vec3 spaceshipDir = glm::vec3(-0.7f, 0.f, -0.7f);

float aspectRatio = 1.f;

// --- Funkcje pomocnicze   ---
glm::mat4 createCameraMatrix()
{
	glm::vec3 cameraSide = glm::normalize(glm::cross(cameraDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 cameraUp = glm::normalize(glm::cross(cameraSide, cameraDir));
	glm::mat4 cameraRotrationMatrix = glm::mat4({
		cameraSide.x,cameraSide.y,cameraSide.z,0,
		cameraUp.x,cameraUp.y,cameraUp.z ,0,
		-cameraDir.x,-cameraDir.y,-cameraDir.z,0,
		0.,0.,0.,1.,
		});
	cameraRotrationMatrix = glm::transpose(cameraRotrationMatrix);
	glm::mat4 cameraMatrix = cameraRotrationMatrix * glm::translate(-cameraPos);
	return cameraMatrix;
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

// --- Funkcje rysujace ---

// Etap 1: Funkcja rysujaca obiekt jednolitym kolorem
void drawObjectColor(Core::RenderContext& context, glm::mat4 modelMatrix, glm::vec3 color) {
	GLuint prog = program; // Uzyj podstawowego programu shaderow
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

// Miejsce na definicje funkcji drawObjectTexture (Etap 2)
void drawObjectTexture(Core::RenderContext& context, glm::mat4 modelMatrix, GLuint colorTextureID, GLuint normalMapTextureID) {
	GLuint prog = programTex; // Uzyj programu shaderow dla tekstur
	glUseProgram(prog);

	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(prog, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(prog, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(prog, "lightPos"), -5.f, 3.f, 3.f);
	glUniform3f(glGetUniformLocation(prog, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	// Ustawienie tekstury jako aktywnej na jednostce 0
	Core::SetActiveTexture(colorTextureID, "colorTexture", prog, 0);
	Core::SetActiveTexture(normalMapTextureID, "normalMap", prog, 1);

	// Miejsce na dodanie aktywacji normal mapy (Etap 5) -> Zostanie dodane w Etapie 5

	Core::DrawContext(context);
	glUseProgram(0);
}

// Miejsce na definicje funkcji drawObjectAtmosphere (Etap 3)
void drawObjectAtmosphere(Core::RenderContext& context, glm::mat4 modelMatrix) {
	GLuint prog = programAtm; // Uzyj programu shaderow atmosfery
	glUseProgram(prog);

	// Wlaczenie blendingu addytywnego
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE); // Kolory beda dodawane
	glDepthMask(GL_FALSE);      // Nie zapisuj do bufora glebokosci, aby nie zaslonic planety

	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(prog, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(prog, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix); // Potrzebne dla normalnych w shaderze
	glUniform3f(glGetUniformLocation(prog, "lightPos"), -5.f, 3.f, 3.f); // Potrzebne dla sunInfluence
	glUniform3f(glGetUniformLocation(prog, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z); // Potrzebne dla rim lighting

	// Uniformy specyficzne dla atmosfery
	glUniform3f(glGetUniformLocation(prog, "atmosphereColor"), 0.35f, 0.57f, 1.0f); // Kolor poswiaty
	glUniform1f(glGetUniformLocation(prog, "intensity"), 1.5f); // Intensywnosc poswiaty

	Core::DrawContext(context);

	// Wylaczenie blendingu i przywrocenie zapisu do bufora glebokosci
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	glUseProgram(0);
}

// Miejsce na definicje funkcji drawObjectClouds (Etap 4)
void drawObjectClouds(Core::RenderContext& context, glm::mat4 modelMatrix, GLuint cloudColorTexture) {
	GLuint prog = programCloud; // Uzyj programu shaderow chmur
	glUseProgram(prog);

	// Wlaczenie standardowego alpha blendingu
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE); // Nie zapisuj do bufora glebokosci

	glm::mat4 viewProjection = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjection * modelMatrix;

	glUniformMatrix4fv(glGetUniformLocation(prog, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(prog, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(prog, "lightPos"), -5.f, 3.f, 3.f); // Dla oswietlenia chmur
	glUniform3f(glGetUniformLocation(prog, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	// Dodaj uniformy, jesli shader ich wymaga (np. shininess)
	glUniform1f(glGetUniformLocation(prog, "shininess"), 30.0f); // Przykladowa wartosc dla specular

	// Ustawienie tekstury koloru/alpha na jednostce 0
	Core::SetActiveTexture(cloudColorTexture, "cloudColor", prog, 0);


	Core::DrawContext(context);

	// Wylaczenie blendingu i przywrocenie zapisu glebokosci
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	glUseProgram(0);
}

// --- Glowna funkcja renderujaca ---
void renderScene(GLFWwindow* window)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	float time = glfwGetTime();

	// Macierz modelu dla planety
	glm::mat4 planetModelMatrix = glm::rotate(time * 0.1f, glm::vec3(0, 1, 0))
		* glm::scale(glm::vec3(3.0) / 110.0f);

	// --- Rysowanie Planety ---
	// Etap 1: Rysuj niebieska kule
	// Ta linia zostanie zastapiona w Etapie 2
	//drawObjectColor(sphereContext, planetModelMatrix, glm::vec3(0.0, 0.3, 1.0));
	// Miejsce na wywolanie rysowania planety z tekstura (Etap 2)

	drawObjectTexture(sphereContext, planetModelMatrix, texture::earth, texture::earthNormal);
	// Miejsce na wywolanie rysowania chmur (Etap 4)
	// Pamietaj o innym skalowaniu i rotacji
	glm::mat4 cloudModelMatrix = glm::rotate(time * 0.05f, glm::vec3(0, 1, 0))
		* planetModelMatrix
		* glm::scale(glm::vec3(1.005f));
	drawObjectClouds(sphereContext, cloudModelMatrix, texture::clouds);

	// Miejsce na wywolanie rysowania atmosfery (Etap 3)
	// Pamietaj o skalowaniu, aby byla wieksza od planety
	glm::mat4 atmosphereModelMatrix = planetModelMatrix * glm::scale(glm::vec3(1.009f));
	drawObjectAtmosphere(sphereContext, atmosphereModelMatrix);

	// --- Rysowanie Statku   ---
	glm::vec3 spaceshipSide = glm::normalize(glm::cross(spaceshipDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 spaceshipUp = glm::normalize(glm::cross(spaceshipSide, spaceshipDir));
	glm::mat4 spaceshipRotationMatrix = glm::mat4({
		spaceshipSide.x,spaceshipSide.y,spaceshipSide.z,0,
		spaceshipUp.x,spaceshipUp.y,spaceshipUp.z ,0,
		-spaceshipDir.x,-spaceshipDir.y,-spaceshipDir.z,0,
		0.,0.,0.,1.,
		});
	glm::mat4 shipModelMatrix = glm::translate(spaceshipPos)
		* spaceshipRotationMatrix
		* glm::eulerAngleY(glm::pi<float>())
		* glm::scale(glm::vec3(1.05f));
	drawObjectColor(shipContext, shipModelMatrix, glm::vec3(0.6, 0.6, 0.7));

	glfwSwapBuffers(window);
}

// --- Funkcje obslugi okna i modeli   ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	aspectRatio = width / float(height);
	glViewport(0, 0, width, height);
}

void loadModelToContext(std::string path, Core::RenderContext& context)
{
	Assimp::Importer import;
	// Wazne: Flaga aiProcess_CalcTangentSpace jest potrzebna dla Etapu 5 (Normal Mapping)
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

// --- Inicjalizacja ---
void init(GLFWwindow* window)
{
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glEnable(GL_DEPTH_TEST);

	// Zaladuj podstawowy program shaderow (Etap 1)
	program = shaderLoader.CreateProgram("shaders/shader_5_1.vert", "shaders/shader_5_1.frag");
	if (program == 0) { 
		std::cerr << "Blad ladowania podstawowych shaderow!" << std::endl; 
		exit(1); 
	}

	// Miejsce na ladowanie dodatkowych programow shaderow (Etapy 2, 3, 4)
	programTex = shaderLoader.CreateProgram("shaders/shader_5_1_tex.vert", "shaders/shader_5_1_tex.frag");
	if (programTex == 0) {
		std::cerr << "Blad ladowania shaderow tekstur!" << std::endl;
		exit(1);
	}

	programAtm = shaderLoader.CreateProgram("shaders/shader_5_1_atm.vert", "shaders/shader_5_1_atm.frag");
	if (programAtm == 0) {
		std::cerr << "Blad ladowania shaderow atmosfery!" << std::endl;
		exit(1);
	}

	programCloud = shaderLoader.CreateProgram("shaders/shader_5_1_cloud.vert", "shaders/shader_5_1_cloud.frag");
	if (programAtm == 0) {
		std::cerr << "Blad ladowania shaderow chmur!" << std::endl;
		exit(1);
	}

	std::cout << "Ladowanie mapy normalnych Ziemi..." << std::endl;
	texture::earthNormal = Core::LoadTexture("textures/Mandalore Legends (Bump 4k).png");
	if (texture::earthNormal == 0) { std::cerr << "Blad ladowania mapy normalnych Ziemi!" << std::endl; }

	std::cout << "Ladowanie mapy bump/normal chmur..." << std::endl;
	texture::cloudsT = Core::LoadTexture("textures/Taris (Clouds Bump 4k).png");
	if (texture::cloudsT == 0) { std::cerr << "Blad ladowania mapy bump/normal chmur!" << std::endl; }

	// Zaladuj modele  
	std::cout << "Ladowanie modelu kuli..." << std::endl;
	loadModelToContext("./models/sphere2.obj", sphereContext);
	std::cout << "Ladowanie modelu statku..." << std::endl;
	loadModelToContext("./models/spaceship.obj", shipContext);


	// Miejsce na ladowanie tekstur (Etapy 2, 4, 5)
	std::cout << "Ladowanie tekstury Ziemi..." << std::endl;
	texture::earth = Core::LoadTexture("textures/Mandalore Legends (Diffuse 4k).png");
	if (texture::earth == 0) {
		std::cerr << "Blad ladowania tekstury Ziemi!" << std::endl;
	}

	std::cout << "Ladowanie tekstury chmur..." << std::endl;
	texture::clouds = Core::LoadTexture("textures/Taris (Clouds 4k).png");
	if (texture::clouds == 0) {
		std::cerr << "Blad ladowania tekstury Chmur!" << std::endl;
	}

	std::cout << "Inicjalizacja zakonczona." << std::endl;
}

// --- Sprzatanie ---
void shutdown(GLFWwindow* window) {
	shaderLoader.DeleteProgram(program);
	// Miejsce na usuwanie dodatkowych programow shaderow (Etapy 2, 3, 4)

	shaderLoader.DeleteProgram(programTex);

	shaderLoader.DeleteProgram(programAtm);

	shaderLoader.DeleteProgram(programCloud);


	// Miejsce na usuwanie tekstur (Etapy 2, 4, 5)
	glDeleteTextures(1, &texture::earth);
	glDeleteTextures(1, &texture::clouds);
	glDeleteTextures(1, &texture::earthNormal);
	glDeleteTextures(1, &texture::cloudsT);
}

// --- Obsluga wejscia   ---
void processInput(GLFWwindow* window)
{
	glm::vec3 spaceshipSide = glm::normalize(glm::cross(spaceshipDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 spaceshipUp = glm::vec3(0.f, 1.f, 0.f);
	float angleSpeed = 0.15f * 0.016f;
	float moveSpeed = 0.3f * 0.016f;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		spaceshipPos += spaceshipDir * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		spaceshipPos -= spaceshipDir * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
		spaceshipPos += spaceshipSide * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
		spaceshipPos -= spaceshipSide * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		spaceshipPos += spaceshipUp * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		spaceshipPos -= spaceshipUp * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		spaceshipDir = glm::vec3(glm::rotate(angleSpeed, spaceshipUp) * glm::vec4(spaceshipDir, 0));
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		spaceshipDir = glm::vec3(glm::rotate(-angleSpeed, spaceshipUp) * glm::vec4(spaceshipDir, 0));

	cameraPos = spaceshipPos - 2.0f * spaceshipDir + glm::vec3(0, 0.8f, 0);
	cameraDir = spaceshipDir;
}

// --- Glowna petla renderowania ---
void renderLoop(GLFWwindow* window) {
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);
		renderScene(window);
		glfwPollEvents();
	}
}
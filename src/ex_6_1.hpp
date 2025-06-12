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

///////// Inicjalizacje zmiennych //////////
// Inicjalizacja zmiennych dla tekstur
namespace texture {
	GLuint earth;
	GLuint clouds;
	GLuint earthNormal;
	GLuint cloudsT;
}

// Inicjalizacja zmiennych dla programow shaderow
GLuint program; // Podstawowy program do rysowania kolorem
GLuint programTex; // Program do rysowania z teksturą
GLuint programAtm; // Program do rysowania atmosfery
GLuint programCloud; // Program do rysowania chmur

Core::Shader_Loader shaderLoader;
	
Core::RenderContext sphereContext;

// Zmienne kamery
glm::vec3 cameraPos = glm::vec3(-6.f, 0, 0);
glm::vec3 cameraDir = glm::vec3(1.f, 0.f, 0.f);
float aspectRatio = 1.f;
float angleSpeed = 0.15f * 0.016f;
float moveSpeed = 0.3f * 0.016f;

float cameraAngleX = 0.0f;
float cameraAngleY = 0.0f;
float cameraDistance = 6.0f;

bool dragging = false;
double lastX = 0.0;
double lastY = 0.0;
float mouseSensitivity = 0.005f;

////////////////////////////////////////////////////

///////// Funkcje do kamery //////////
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
////////////////////////////////////////////////////


///////// Główna funkcja renderująca /////////
void renderScene(GLFWwindow* window)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	float time = glfwGetTime();

	// Macierz modelu dla planety
	glm::mat4 planetModelMatrix = glm::scale(glm::vec3(3.0) / 110.0f);

	// Rysowanie planety
	drawObjectTexture(sphereContext, planetModelMatrix, texture::earth, texture::earthNormal);

	// Macierz modelu dla chmur
	glm::mat4 cloudModelMatrix = planetModelMatrix
		* glm::scale(glm::vec3(1.005f));

	// Rysowanie chmur
	drawObjectClouds(sphereContext, cloudModelMatrix, texture::clouds);

	// Macierz modelu dla atmosfery
	glm::mat4 atmosphereModelMatrix = planetModelMatrix * glm::scale(glm::vec3(1.009f));
	
	// Rysowanie atmosfery
	drawObjectAtmosphere(sphereContext, atmosphereModelMatrix);

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

///////// Funkcja inicjalizująca /////////
void init(GLFWwindow* window)
{
	glfwSetMouseButtonCallback(window, [](GLFWwindow* w, int button, int action, int mods) {
		ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplGlfw_MouseButtonCallback(w, button, action, mods); // Forward to ImGui

		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (action == GLFW_PRESS && !io.WantCaptureMouse) {
				dragging = true;
				glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				glfwGetCursorPos(w, &lastX, &lastY);
			}
			else if (action == GLFW_RELEASE) {
				dragging = false;
				glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
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
	if (programAtm == 0) {
		std::cerr << "Blad ladowania shaderow chmur!" << std::endl;
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
}

///////// Czyszczenie po zamknięciu /////////
void shutdown(GLFWwindow* window) {
	shaderLoader.DeleteProgram(program);
	shaderLoader.DeleteProgram(programTex);
	shaderLoader.DeleteProgram(programAtm);
	shaderLoader.DeleteProgram(programCloud);

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

	if  (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
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

		ImGui::SetNextWindowSize(ImVec2(120, 200), ImGuiCond_Always);
		ImGui::Begin("Sterowanie");

		ImGui::PushItemWidth(-1);
		// Slidery
		ImGui::SliderFloat("angleSpeed", &angleSpeed, 0.0f, 0.3f);
		ImGui::SliderFloat("zoomSpeed", &moveSpeed, 0.0f, 0.3f);
		ImGui::PopItemWidth();

		ImGui::End();

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
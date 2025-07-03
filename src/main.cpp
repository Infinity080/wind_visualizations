#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "glew.h"
#include <GLFW/glfw3.h>
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include "Wind_App.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;



int main(int argc, char** argv)
{
	// Inicjalizacja glfw
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// Tworzenie okna za pomoc¹ glfw
	GLFWwindow* window = glfwCreateWindow(1000, 1000, u8"Wizualizacja Wiatrów Œwiata", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// £adowanie OpenGL za pomoc¹ glew
	glewInit();
	glViewport(0, 0, 1000, 1000);

	// £adowanie ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	

	ImGui::StyleColorsDark();

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

	colors[ImGuiCol_WindowBg] = ImVec4(0.078f, 0.129f, 0.239f, 1.00f);           

	colors[ImGuiCol_ChildBg] = ImVec4(0.078f, 0.129f, 0.239f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.078f, 0.129f, 0.239f, 1.00f);

	colors[ImGuiCol_Border] = ImVec4(0.0f, 0.71f, 0.847f, 0.50f);                   

	colors[ImGuiCol_FrameBg] = ImVec4(0.178f, 0.229f, 0.339f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.0f, 0.71f, 0.847f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.0f, 0.71f, 0.847f, 1.00f);

	colors[ImGuiCol_TitleBg] = ImVec4(0.078f, 0.129f, 0.239f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.71f, 0.847f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.078f, 0.129f, 0.239f, 0.75f);

	colors[ImGuiCol_Button] = ImVec4(0.0f, 0.71f, 0.847f, 0.40f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.282f, 0.792f, 0.894f, 0.80f);     
	colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.71f, 0.847f, 1.00f);

	colors[ImGuiCol_Header] = ImVec4(0.0f, 0.71f, 0.847f, 0.40f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.282f, 0.792f, 0.894f, 0.80f);        
	colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.71f, 0.847f, 1.00f);

	colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.71f, 0.847f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.282f, 0.792f, 0.894f, 1.00f);       
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 0.71f, 0.847f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

	colors[ImGuiCol_Separator] = ImVec4(0.0f, 0.71f, 0.847f, 0.50f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.282f, 0.792f, 0.894f, 0.78f);     
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.0f, 0.71f, 0.847f, 1.00f);

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 12.0f;
	style.ChildRounding = 12.0f;
	style.GrabRounding = 12.0f;
	style.FrameRounding = 12.0f;

	ImGuiIO& io = ImGui::GetIO();

	static const ImWchar polish_ranges[] = {
	0x0020, 0x00FF,
	0x0100, 0x017F,
	0x0180, 0x024F,
	0,
	};

	
	ImFont* segoeFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 24.0f, NULL, polish_ranges);
	if (segoeFont) io.FontDefault = segoeFont;

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, false);
	ImGui_ImplOpenGL3_Init("#version 430");

	init(window);

	// Uruchomienie g³ównej pêtli
	renderLoop(window);

	shutdown(window);
	glfwTerminate();
	return 0;
}

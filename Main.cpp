#include <iostream>
#include <string>
#include <algorithm>

#define GLEW_STATIC
#include "glew.h"
#include "glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Application.h"
#include "Style.h"


int main(void) 
{
    if (!glfwInit()) 
    {
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Serial Monitor", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform Windows

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    Style::Dracula();
    /* ******************* */

    /* Change Font */
    ImFont *pFont = io.Fonts->AddFontFromFileTTF("..\\fonts\\Jetbrains.ttf",24);
    /* *********************************************************************/

    // Main loop
    while (!glfwWindowShouldClose(window)) 
    {
        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::PushFont(pFont);
        
        MyApp::RenderUI(window);
        ImGui::ShowDemoWindow();

        /* *************************************************************************************** */

        /* Code ends  here */
        ImGui::PopFont();

        // Rendering
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

#include <iostream>
#include <string>
#include <algorithm>

#define GLEW_STATIC
#include "glew.h"
#include "glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "glfw3native.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "Application.h"
#include "Style.h"

#ifdef _WIN32
    #include <uxtheme.h>
    #include <windows.h>
    #include <dwmapi.h>
#endif

double s_xpos = 0, s_ypos = 0;
int w_xsiz = 0, w_ysiz = 0;
int dragState = 0;

void attemptDragWindow(GLFWwindow* window) 
{
	if (glfwGetMouseButton(window, 0) == GLFW_PRESS && dragState == 0) {
		glfwGetCursorPos(window, &s_xpos, &s_ypos);
		glfwGetWindowSize(window, &w_xsiz, &w_ysiz);
		dragState = 1;
	}
	if (glfwGetMouseButton(window, 0) == GLFW_PRESS && dragState == 1) {
		double c_xpos, c_ypos;
		int w_xpos, w_ypos;
		glfwGetCursorPos(window, &c_xpos, &c_ypos);
		glfwGetWindowPos(window, &w_xpos, &w_ypos);
		if (
			s_xpos >= 0 && s_xpos <= ((double)w_xsiz - 170) &&
			s_ypos >= 0 && s_ypos <= 25) {
			glfwSetWindowPos(window, w_xpos + (c_xpos - s_xpos), w_ypos + (c_ypos - s_ypos));
		}
		if (
			s_xpos >= ((double)w_xsiz - 15) && s_xpos <= ((double)w_xsiz) &&
			s_ypos >= ((double)w_ysiz - 15) && s_ypos <= ((double)w_ysiz)) {
			glfwSetWindowSize(window, w_xsiz + (c_xpos - s_xpos), w_ysiz + (c_ypos - s_ypos));
		}
	}
	if (glfwGetMouseButton(window, 0) == GLFW_RELEASE && dragState == 1) {
		dragState = 0;
	}
}

#ifdef WINDOWS
WNDPROC original_proc;
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg)
    {
        case WM_NCCALCSIZE:
        {
            // Remove the window's standard sizing border
            if (wParam == TRUE && lParam != NULL)
            {
                NCCALCSIZE_PARAMS* pParams = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
                pParams->rgrc[0].top += 1;
                pParams->rgrc[0].right -= 2;
                pParams->rgrc[0].bottom -= 2;
                pParams->rgrc[0].left += 2;
            }
            return 0;
        }
        case WM_NCPAINT:
        {
            // Prevent the non-client area from being painted
            return 0;
        }
        case WM_NCHITTEST:
        {
            // Expand the hit test area for resizing
            const int borderWidth = 8; // Adjust this value to control the hit test area size

            POINTS mousePos = MAKEPOINTS(lParam);
            POINT clientMousePos = { mousePos.x, mousePos.y };
            ScreenToClient(hWnd, &clientMousePos);

            RECT windowRect;
            GetClientRect(hWnd, &windowRect);
            
            // Calculate window width
            int windowWidth = windowRect.right - windowRect.left;
            
            // Define middle draggable area
            int dragAreaWidth = 200;  // Adjust this value to your needs
            int dragAreaLeft = (windowWidth - dragAreaWidth) / 2;
            int dragAreaRight = dragAreaLeft + dragAreaWidth;
            
            if (clientMousePos.y >= windowRect.top && clientMousePos.y < windowRect.top + 32 &&
                clientMousePos.x >= windowRect.left + dragAreaLeft && 
                clientMousePos.x <= windowRect.left + dragAreaRight)
            {
                return HTCAPTION;
            }
            
            if (clientMousePos.y >= windowRect.bottom - borderWidth)
            {
                if (clientMousePos.x <= borderWidth)
                    return HTBOTTOMLEFT;
                else if (clientMousePos.x >= windowRect.right - borderWidth)
                    return HTBOTTOMRIGHT;
                else
                    return HTBOTTOM;
            }
            else if (clientMousePos.y <= borderWidth)
            {
                if (clientMousePos.x <= borderWidth)
                    return HTTOPLEFT;
                else if (clientMousePos.x >= windowRect.right - borderWidth)
                    return HTTOPRIGHT;
                else
                    return HTTOP;
            }
            else if (clientMousePos.x <= borderWidth)
            {
                return HTLEFT;
            }
            else if (clientMousePos.x >= windowRect.right - borderWidth)
            {
                return HTRIGHT;
            }
            break;
        }
    }
    return CallWindowProc(original_proc, hWnd, uMsg, wParam, lParam);
}
#endif

void disableTitlebar(GLFWwindow* window)
{
#ifdef WINDOWS
    HWND hWnd = glfwGetWin32Window(window);

    LONG_PTR lStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
    lStyle |= WS_THICKFRAME;
    lStyle &= ~WS_CAPTION;
    SetWindowLongPtr(hWnd, GWL_STYLE, lStyle);

    RECT windowRect;
    GetWindowRect(hWnd, &windowRect);
    int width = windowRect.right - windowRect.left;
    int height = windowRect.bottom - windowRect.top;

    original_proc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
    (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc));
    SetWindowPos(hWnd, NULL, 0, 0, width, height, SWP_FRAMECHANGED | SWP_NOMOVE);
#endif
}

void DarkTitleBar(GLFWwindow* window)
{
    HWND hwnd = glfwGetWin32Window(window);
    BOOL value = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    SetWindowTheme(hwnd, L"DarkMode_Explorer", NULL);
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

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

    // disableTitlebar(window);
    DarkTitleBar(window);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

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

        // attemptDragWindow(window);

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::PushFont(pFont);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{15.0f,10.0f});
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 15.0f);

        
        MyApp::RenderUI(window);
        ImGui::ShowDemoWindow();
        ImPlot::ShowDemoWindow();

        /* *************************************************************************************** */

        /* Code ends  here */
        ImGui::PopFont();
        ImGui::PopStyleVar(4);

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

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

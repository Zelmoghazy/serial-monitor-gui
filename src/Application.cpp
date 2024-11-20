#include <atomic>
#include <bitset>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <format>
#include <thread>

#include "glew.h"
#include "glfw3.h"
#include "Application.h"
#include "imgui.h"
#include "implot.h"
#include "imgui_internal.h"
#include "Platform_Utils.h"
#include "Image.h"

#include "serial.hpp"
#include "utils.hpp"


bool utils;
bool demo;
bool image;
bool table;
bool input;

double multipliers[10][10] = 
{
    // From bits to:    bits    bytes   kilobits    kilobytes   megabits   megabytes   gigabits   gigabytes   terabytes   petabytes
    /* bits */        {1.0,   0.125, 0.001,    0.000125, 1e-6, 1.25e-7, 1e-9, 1.25e-10, 1.25e-13, 1.25e-16},
    
    // From bytes to: 
    /* bytes */       {8.0,   1.0,   0.008, 0.001, 8.0e-6, 1.0e-06, 8.0e-9, 1.0e-9, 1.0e-12, 1.0e-15},
    
    // From kilobits to:
    /* kilobits */    {1000.0, 125.0, 1.0,    0.125,    0.001,    0.000125,   1.0e-6,    1.25e-7,  1.25e-10, 1.25e-13},
    
    // From kilobytes to:
    /* kilobytes */   {8000.0, 1000.0, 8.0, 1.0, 0.008, 0.001, 8.0e-6, 1.0e-6,  1.0e-9, 1e-12},
    
    // From megabits to:
    /* megabits */    {1.0e6,   125000.0, 1000.0, 125.0, 1.0,  0.125,  0.001,   0.000125,   1.25e-7,  1.25e-10},
    
    // From megabytes to:
    /* megabytes */   {8.0e6,   1.0e6, 8000.0, 1000.0, 8.0,   1.0, 0.008,   0.001, 1.0e-6,  1.0e-9},
    
    // From gigabits to:
    /* gigabits */    {1.0e9,   125.0e6, 1e6, 125000.0, 1000.0, 125.0, 1.0, 0.125, 1.25e-4,  1.25e-7},
    
    // From gigabytes to:
    /* gigabytes */   {8.0e9f,   1.0e9f, 8e6f, 1e6f, 8000.0f, 1000.0f, 8.0f,   1.0f, 0.001f,  1e-6f},
    
    // From terabytes to:
    /* terabytes */   {8.0e12,  8.0e12, 8.0e9, 1.0e9, 8.0e6, 1.0e6, 8000.0, 1000.0, 1.0, 0.001},
    
    // From petabytes to:
    /* petabytes */   {8.0e15,  1.0e15, 8.0e12, 1.0e12, 8.0e9, 1.0e9, 8.0e6, 1.0e6, 1000, 1.0}
};

char bases[4][1024];

namespace MyApp
{
class SigrokDataCapture {
private:
    std::string driver;
    std::string output_file;
    long sample_rate;
    std::vector<std::string> channels;
    std::vector<double> captured_data;
    std::atomic<bool> is_capturing{false};
    std::mutex data_mutex;
    std::string sigrok_cli_path;

public:
    SigrokDataCapture(
        const std::string& drv = "fx2lafw", 
        const std::string& out_file = "capture.bin", 
        long rate = 2000000,
        const std::string& cli_path = R"(C:\Program Files\sigrok\sigrok-cli\sigrok-cli.exe)"
    ) : 
        driver(drv), 
        output_file(out_file), 
        sample_rate(rate),
        sigrok_cli_path(cli_path) {}

    bool start_capture() {
        // Prevent multiple simultaneous captures
        if (is_capturing) {
            std::cerr << "Capture already in progress" << std::endl;
            return false;
        }

        is_capturing = true;
        std::thread capture_thread([this]() {
            try {
                // Windows-friendly capture command
                // Remove piping and external tools for Windows
                std::string cmd = "\"" + sigrok_cli_path + "\" " +
                    "--driver " + driver + " " +
                    "--output-file " + output_file + " " +
                    "--output-format binary " +
                    "--config samplerate=" + std::to_string(sample_rate) + " " +
                    "--continuous";

                std::cout << "Executing capture command: " << cmd << std::endl;
                int result = system(cmd.c_str());

                // Update capture status
                is_capturing = false;
            } catch (const std::exception& e) {
                std::cerr << "Capture error: " << e.what() << std::endl;
                is_capturing = false;
            }
        });
        capture_thread.detach();

        return true;
    }

    bool load_binary_data() {
        std::lock_guard<std::mutex> lock(data_mutex);
        
        // Check if file exists
        if (!std::filesystem::exists(output_file)) {
            std::cerr << "File does not exist: " << output_file << std::endl;
            return false;
        }

        std::ifstream file(output_file, std::ios::binary);
        if (!file) {
            std::cerr << "Cannot open file: " << output_file << std::endl;
            return false;
        }

        // Read binary data
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        // Adjust data type if needed based on your specific capture format
        captured_data.resize(file_size / sizeof(double));
        file.read(reinterpret_cast<char*>(captured_data.data()), file_size);

        return !file.fail();
    }
    static bool getter(void* data, int idx, double* out_value) {
        auto* captured_data = reinterpret_cast<std::vector<double>*>(data);
        if (idx < 0 || idx >= captured_data->size()) {
            return false;
        }
        *out_value = captured_data->at(idx);
        return true;
    }

        void plot_data() {
                // Main window
                ImGui::Begin("Sigrok Data Capture Control", nullptr, 
                    ImGuiWindowFlags_AlwaysAutoResize);

                // Capture Button
                if (ImGui::Button("Start Capture", ImVec2(200, 50))) {
                    start_capture();
                }
                ImGui::SameLine();
                ImGui::Text(is_capturing ? "Capturing..." : "Idle");

                // Load Data Button
                if (ImGui::Button("Load Captured Data", ImVec2(200, 50))) {
                    if (load_binary_data()) {
                        ImGui::OpenPopup("Data Loaded");
                    } else {
                        ImGui::OpenPopup("Load Failed");
                    }
                }

                // Load Success/Failure Popups
                if (ImGui::BeginPopupModal("Data Loaded", nullptr, 
                    ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Binary data successfully loaded!");
                    if (ImGui::Button("Close")) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                if (ImGui::BeginPopupModal("Load Failed", nullptr, 
                    ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Failed to load binary data!");
                    if (ImGui::Button("Close")) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                char label[20];
                // Data Plot
                ImGui::Begin("Captured Data Visualization");
                if (!captured_data.empty()) {
                    if (ImPlot::BeginPlot("Captured Samples", 
                        ImVec2(-1, 400))) {  // Full-width plot
                        ImPlot::PlotLine("Data", 
                            captured_data.data(), 
                            captured_data.size());
                        ImPlot::EndPlot();
                    }

                    if (ImPlot::BeginPlot("##Digital")) {
                        ImPlot::SetupAxisLimits(ImAxis_Y1, -1, 1);
                            snprintf(label, sizeof(label), "digital");
                            ImPlot::PlotDigitalG("Captured Data", getter, &captured_data, captured_data.size(), ImPlotDigitalFlags_None);
                        ImPlot::EndPlot();
                    }

                    // Additional data info
                    ImGui::Text("Total Samples: %zu", captured_data.size());
                    ImGui::Text("Min Value: %f", *std::min_element(captured_data.begin(), captured_data.end()));
                    ImGui::Text("Max Value: %f", *std::max_element(captured_data.begin(), captured_data.end()));
                } else {
                    ImGui::Text("No data captured. Click 'Start Capture' and then 'Load Captured Data'.");
                }

                ImGui::End();
                ImGui::End();  // Control window

            }
        };

    void ToggleButton(const char *str_id, bool *v)
    {
        ImVec4 *colors = ImGui::GetStyle().Colors;
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        float height = ImGui::GetFrameHeight();
        float width = height * 1.55f;
        float radius = height * 0.50f;

        ImGui::InvisibleButton(str_id, ImVec2(width, height));
        if (ImGui::IsItemClicked())
            *v = !*v;
        ImGuiContext &gg = *GImGui;
        float ANIM_SPEED = 0.085f;
        if (gg.LastActiveId == gg.CurrentWindow->GetID(str_id)) // && g.LastActiveIdTimer < ANIM_SPEED)
            float t_anim = ImSaturate(gg.LastActiveIdTimer / ANIM_SPEED);
        if (ImGui::IsItemHovered())
            draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::GetColorU32(*v ? colors[ImGuiCol_ButtonActive] : ImVec4(0.78f, 0.78f, 0.78f, 1.0f)), height * 0.5f);
        else
            draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::GetColorU32(*v ? colors[ImGuiCol_Button] : ImVec4(0.85f, 0.85f, 0.85f, 1.0f)), height * 0.50f);
        draw_list->AddCircleFilled(ImVec2(p.x + radius + (*v ? 1 : 0) * (width - radius * 2.0f), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));
    }

    void minimizeWindow(GLFWwindow* window) 
    {
        // Get the Win32 window handle from GLFW window
        HWND hwnd = glfwGetWin32Window(window);
    
        // Use Win32 ShowWindow function to minimize
        ShowWindow(hwnd, SW_MINIMIZE);
    }
    
    void DockSpace(bool *p_open, GLFWwindow* window)
    {
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
        // and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
        // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
        // all active windows docked into it will lose their parent and become undocked.
        // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
        // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("Serial Monitor", p_open, window_flags);
        
        ImGui::PopStyleVar(3);

        ImGuiIO &io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
                if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
                ImGui::Separator();
                if (ImGui::MenuItem("Cut", "CTRL+X")) {}
                if (ImGui::MenuItem("Copy", "CTRL+C")) {}
                if (ImGui::MenuItem("Paste", "CTRL+V")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("utils", "CTRL+Z")) {utils^=1;}
                if (ImGui::MenuItem("demo", "CTRL+Y", false, false)) {demo^=1;}  // Disabled item
                ImGui::Separator();
                if (ImGui::MenuItem("image", "CTRL+X")) {image^=1;}
                if (ImGui::MenuItem("table", "CTRL+C")) {table^=1;}
                if (ImGui::MenuItem("input", "CTRL+V")) {input^=1;}
                ImGui::EndMenu();
            }
            if(ImGui::Button("-"))
            {
                minimizeWindow(window);
            }
            if(ImGui::Button("X"))
            {
                glfwSetWindowShouldClose(window, true);
                glfwDestroyWindow(window);
                glfwTerminate();
            }
            ImGui::EndMainMenuBar();
        }
        ImGui::End();
    }

    void loadfile(const char *path, char *source)
    {
        /* Reading */
        FILE *reading_file = fopen(path, "r");
        assert(reading_file);
        /* Doesnt return accurate size */
        fseek(reading_file, 0, SEEK_END);
        size_t size = (size_t)ftell(reading_file);
        fseek(reading_file, 0, SEEK_SET);

        if(size > 1024 * 16){
            strcpy(source,"Source code exceeds size limit use the command line utility instead.");
            fclose(reading_file);
            return;
        }
        size = 0;
        int ch = fgetc(reading_file);
        /* read input character by character */
        while (ch!=EOF)
        {
            source[size++]=(char)ch;
            ch = fgetc(reading_file);
        }
        source[size++] = '\n';     // Always insert a newline at the end.
        source[size]   = '\0';
        fclose(reading_file);
    }

    void tokenizeLine(const std::string& line, std::string& identifier, std::string& token) 
    {
        std::istringstream iss(line);
        std::getline(iss, token, ',');
        std::getline(iss, identifier, ',');
            // Check if the first and second characters are both ','
        if (token.size() >= 2 && token[0] == ',' && token[1] == ',') {
            // Append ',' to the token
            token += ',';
            identifier = "ERROR";
        }
    }

    // Function to read the file and populate vectors
    void readFile(const std::string& filename, std::vector<std::string>& identifiers, std::vector<std::string>& tokens) 
    {
        std::ifstream file(filename);
        std::string line;

        while (std::getline(file, line)) 
        {
            if (line.empty()) {
                continue;
            }
            std::string identifier, token;
            tokenizeLine(line, identifier, token);
            // Push the values into vectors
            identifiers.push_back(identifier);
            tokens.push_back(token);
        }
        file.close();
    }

    std::string runProgramAndGetOutput(const char* command) 
    {
        // FILE* pipe = popen(command, "r");
        // if (!pipe) {
        //     return "Error: Unable to open pipe.";
        // }
        // char buffer[128];
        std::string result = "";
        // while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        //     result += buffer;
        // }
        // int status = pclose(pipe);
        // if (status == -1) {
        //     return "Error: Failed to close pipe.";
        // }
        return result;
    }

    std::string removeEmptyLines(const std::string &input) 
    {
        std::istringstream iss(input);
        std::ostringstream oss;
    
        std::string line;
        while (std::getline(iss, line)) {
            // Check if the line is not empty
            if (!line.empty()) {
                oss << line << '\n';
            }
        }
        return oss.str();
    }

    void HexConverterWindow(const char *hexInput) 
    {
        static std::string decimalOutput = "";
        static std::string binaryOutput = "";

        if (ImGui::Button("Convert")) {
            // Clear previous results
            decimalOutput.clear();
            binaryOutput.clear();

            // Parse hexadecimal input
            std::stringstream ss;
            ss << std::hex << hexInput;
            unsigned long decimalValue = 0;
            if (!(ss >> decimalValue)) {
                decimalOutput = "Invalid input";
                binaryOutput = "Invalid input";
            } else {
                // Convert to decimal
                decimalOutput = std::to_string(decimalValue);

                // Convert to binary
                binaryOutput = std::bitset<64>(decimalValue).to_string();
                binaryOutput.erase(0, binaryOutput.find_first_not_of('0'));  // Remove leading zeros
            }
        }

        ImGui::Text("Decimal: %s", decimalOutput.c_str());
        ImGui::Text("Binary: %s", binaryOutput.c_str());
    }

    void UpdateUnits(double* amount, int changedIndex) 
    {
        double baseValue = amount[changedIndex];
        for (int i = 0; i < 10; i++) {
            if (i != changedIndex) {
                amount[i] = baseValue * multipliers[changedIndex][i];
            }
        }
    }

    void ConvertBases(int base)
    {
        long long number = 0;
        switch(base){
            case 10:
                number = strtoll(bases[0],nullptr,10);
                break; 
            case 16:
                number = strtoll(bases[1],nullptr, 16);
                break;
            case 8:
                number = strtoll(bases[2],nullptr, 8);
                break;
            case 2:
                number = strtoll(bases[3],nullptr, 2);
                break;
        }
        sprintf(bases[0],"%lld",number);
        sprintf(bases[1],"%llX",number);
        sprintf(bases[2],"%llo",number);
        strcpy(bases[3],std::bitset<64>(number).to_string().c_str());
    }

    void RenderUI(GLFWwindow* window)
    {
        /* Variables */
        static bool checkboxval = false;
        static int  Multiplier  = 3;
        static int  text_input_size = 10;
        static char text_input[1024 * 16] = "Enter your source code here \n";

        /* Windows file dialog */
        static std::string terminal_output;
        static std::string file_path;

        /* Loading an Image */ 
        static std::string path = "..\\data\\BG.png";
        static Image img(path.c_str());
        static bool loaded = true;

        /* Table stuff */
        static std::vector<std::string> identifiers;
        static std::vector<std::string> tokens;

        static ImGuiTextBuffer log;
        static ImGuiTextFilter filter;
        static ImVector<int>   line_off; // Index to lines offset. We maintain this with AddLog() calls.

        static serial_handle comm{"COM3", baud_rate_e::BAUDRATE_9600};
        static std::vector<std::string> ports = comm.get_available_ports();
        
        static bool success_flag = false;


        static char temp_line[1024];
        static int temp_line_idx = 0;

        static int old_size = 0;
        static int lines = 0;
        static bool ScrollToBottom = false;
        static bool AutoScroll = false;

        static bool start = true;

        static SigrokDataCapture capture;

        if(start)
        {
            log.clear();
            line_off.clear();
            line_off.push_back(0);
            start = false;
        }

        /* Docking Space */ 
        DockSpace(nullptr, window);

        /* ***************** */

        capture.plot_data();

        bool p_open;

        if(utils)
        {
        ImGui::Begin("Utilities", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
        {
            static char buf3[32] = ""; 
            ImGui::InputText("hexadecimal", buf3, 32, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);

            if (buf3[0] != '\0') {
                // Parse input into bytes
                size_t len = strlen(buf3);
                if (len % 2 == 0) { // Ensure input length is even
                    ImGui::Text("Big-endian: %s", buf3);

                    // Convert to little-endian by reversing byte pairs
                    char littleEndian[32] = "";
                    for (size_t i = 0; i < len; i += 2) {
                        littleEndian[i] = buf3[len - 2 - i];
                        littleEndian[i + 1] = buf3[len - 1 - i];
                    }
                    littleEndian[len] = '\0';

                    ImGui::Text("Little-endian: %s", littleEndian);
                }
            }

            HexConverterWindow(buf3);

        /* if (ImGui::TreeNode("Other"))
            {
                ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
                if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
                {
                    if (ImGui::BeginTabItem("Avocado"))
                    {
                        ImGui::Text("This is the Avocado tab!\nblah blah blah blah blah");
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Broccoli"))
                    {
                        ImGui::Text("This is the Broccoli tab!\nblah blah blah blah blah");
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Cucumber"))
                    {
                        ImGui::Text("This is the Cucumber tab!\nblah blah blah blah blah");
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
                ImGui::Separator();
                ImGui::TreePop();
            } */

            static bool animate = true;
            ImGui::Checkbox("Animate", &animate);

            // Fill an array of contiguous float values to plot
            // Tip: If your float aren't contiguous but part of a structure, you can pass a pointer to your first float
            // and the sizeof() of your structure in the "stride" parameter.
            static float values[90] = {};
            static int values_offset = 0;
            static double refresh_time = 0.0;
            if (!animate || refresh_time == 0.0)
                refresh_time = ImGui::GetTime();
            while (refresh_time < ImGui::GetTime()) // Create data at fixed 60 Hz rate for the demo
            {
                static float phase = 0.0f;
                values[values_offset] = cosf(phase);
                values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
                phase += 0.10f * values_offset;
                refresh_time += 1.0f / 60.0f;
            }

            // Plots can display overlay texts
            // (in this example, we will display an average value)
            {
                float average = 0.0f;
                for (int n = 0; n < IM_ARRAYSIZE(values); n++)
                    average += values[n];
                average /= (float)IM_ARRAYSIZE(values);
                char overlay[32];
                sprintf(overlay, "avg %f", average);
                ImGui::PlotLines("Lines", values, IM_ARRAYSIZE(values), values_offset, overlay, -1.0f, 1.0f, ImVec2(0, 80.0f));
            }
        }
        ImGui::End();
        }
        
        ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("TTY", nullptr, ImGuiWindowFlags_NoCollapse))
        {
            ImGui::End();
            return;
        }
            if (ImGui::CollapsingHeader("Configuration", ImGuiTreeNodeFlags_None))
            {
                if(ImGui::Button("Reload"))
                {
                    ports = comm.get_available_ports();
                    for (const auto& port : ports) {
                        std::cout << port << std::endl;
                    }
                }
                
                static ImGuiComboFlags flags = 0;
                flags &= ~ImGuiComboFlags_NoPreview;

                static int port_current_idx = 0; 
                const char* port_preview_value = "No Ports are detected.."; 
                if(ports.size()>0){
                    port_preview_value = ports[port_current_idx].c_str();
                }

                if (ImGui::BeginCombo("Port", port_preview_value, flags))
                {
                    for (int n = 0; n < ports.size(); n++)
                    {
                        const bool is_selected = (port_current_idx == n);
                        if (ImGui::Selectable(ports[n].c_str(), is_selected))
                            port_current_idx = n;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                static int baudrate_current_idx = 0; 
                const char* baudrate_preview_value = BAUD_RATE_STRINGS[baudrate_current_idx];
                if (ImGui::BeginCombo("Baudrate", baudrate_preview_value, flags))
                {
                    for (int n = 0; n < static_cast<int>(baud_rate_e::BAUDRATE_COUNT); n++)
                    {
                        const bool is_selected = (baudrate_current_idx == n);
                        if (ImGui::Selectable(BAUD_RATE_STRINGS[n], is_selected))
                            baudrate_current_idx = n;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                static int parity_current_idx = 0; // Here we store our selection data as an index.
                const char* parity_preview_value = PARITY_STRINGS[parity_current_idx];  // Pass in the preview value visible before opening the combo (it could be anything)
                if (ImGui::BeginCombo("Parity", parity_preview_value, flags))
                {
                    for (int n = 0; n < static_cast<int>(parity_e::P_COUNT); n++)
                    {
                        const bool is_selected = (parity_current_idx == n);
                        if (ImGui::Selectable(PARITY_STRINGS[n], is_selected))
                            parity_current_idx = n;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                static int stop_current_idx = 0; // Here we store our selection data as an index.
                const char* stop_preview_value = STOP_BITS_STRINGS[stop_current_idx];  // Pass in the preview value visible before opening the combo (it could be anything)
                if (ImGui::BeginCombo("Stop Bits", stop_preview_value, flags))
                {
                    for (int n = 0; n < static_cast<int>(stop_bits_e::STOP_COUNT); n++)
                    {
                        const bool is_selected = (stop_current_idx == n);
                        if (ImGui::Selectable(STOP_BITS_STRINGS[n], is_selected))
                            stop_current_idx = n;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                static int data_current_idx = 0; // Here we store our selection data as an index.
                const char* data_preview_value = DATA_BITS_STRINGS[data_current_idx];  // Pass in the preview value visible before opening the combo (it could be anything)
                if (ImGui::BeginCombo("Data Bits", data_preview_value, flags))
                {
                    for (int n = 0; n < static_cast<int>(data_e::DATA_COUNT); n++)
                    {
                        const bool is_selected = (data_current_idx == n);
                        if (ImGui::Selectable(DATA_BITS_STRINGS[n], is_selected))
                            data_current_idx = n;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                if(ImGui::Button("Connect"))
                {
                    if(comm.is_connection_open()){
                        comm.close_connection();
                    }
                    if(ports[port_current_idx].size() == 0){
                        ImGui::Text("Choose a Serial Port");
                    }else{
                        comm = serial_handle(ports[port_current_idx], static_cast<baud_rate_e>(baudrate_current_idx), static_cast<data_e>(data_current_idx), static_cast<parity_e>(parity_current_idx), static_cast<stop_bits_e>(stop_current_idx), false);

                        if (comm.open_connection() == 0){
                            printf("Connection Open\n");
                        }else{
                            printf("Connection Failed\n");
                        }
                    }
                }
            }
            {
                ImGui::SeparatorText("Console");
                filter.Draw("Filter", -100.0f);
                ImGui::Text("Buffer contents: %d lines, %d bytes", lines, log.size());

                if (ImGui::Button("Clear")) 
                {
                    log.clear();
                    line_off.clear();
                    line_off.push_back(0);
                    lines = 0;
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Copy")) 
                {
                    ImGui::SetClipboardText(log.c_str());
                }
                ImGui::SameLine();

                char ch = comm.read_char(success_flag);

                ImGui::Checkbox("Auto-scroll", &AutoScroll);
                ImGui::SameLine();
                if (ImGui::Button("Scroll to bottom")){
                    ScrollToBottom = true;
                }

                if(success_flag) 
                {
                    if(ch == '\n') {
                        temp_line[temp_line_idx] = '\0';
                        log.append(temp_line);
                        log.append("\n");
                        lines++;
                        temp_line_idx = 0;
                        old_size = log.size();
                        line_off.push_back(old_size);
                    }else{
                        temp_line[temp_line_idx++] = ch;
                    }
                }

                ImGui::BeginChild("##log", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                            const char* buf = log.begin();
                            const char* buf_end = log.end();
                            if (filter.IsActive())
                            {
                                for (int line_no = 0; line_no < line_off.Size; line_no++)
                                {
                                    const char* line_start = buf + line_off[line_no];
                                    const char* line_end = (line_no + 1 < line_off.Size) ? (buf + line_off[line_no + 1] - 1) : buf_end;
                                    if (filter.PassFilter(line_start, line_end))
                                        ImGui::TextUnformatted(line_start, line_end);
                                }
                            }
                            else
                            {
                                // ImGui::TextUnformatted(log.begin(), log.end());
                                ImGuiListClipper clipper;
                                clipper.Begin(line_off.Size);
                                while (clipper.Step())
                                {
                                    for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                                    {
                                        const char* line_start = buf + line_off[line_no];
                                        const char* line_end = (line_no + 1 < line_off.Size) ? (buf + line_off[line_no + 1] - 1) : buf_end;
                                        ImGui::TextUnformatted(line_start, line_end);
                                    }
                                }
                                clipper.End();
                            }       
                        ImGui::PopStyleVar();
                        if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())){
                            ImGui::SetScrollHereY(1.0f);
                            ScrollToBottom = false;
                        }
                    ImGui::PopStyleColor();
                ImGui::EndChild();
            }  

        ImGui::End();

        /* First Window */
        if(input)
        {
        ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::Text("Enter your source code or browse from a file");

            /* Text Box */
            static ImGuiInputTextFlags text_flags = ImGuiInputTextFlags_ReadOnly;
            ImGui::SliderInt("Input size", &text_input_size, 1, 50);
            /* Text Box Properties */
            ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX()+100 ,ImGui::GetCursorPosY()));
            ImGui::CheckboxFlags("ReadOnly", &text_flags, ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine(0, 20);
            ImGui::CheckboxFlags("AllowTabInput", &text_flags, ImGuiInputTextFlags_AllowTabInput);
            ImGui::SameLine(0, 20);
            ImGui::CheckboxFlags("CtrlEnterForNewLine", &text_flags, ImGuiInputTextFlags_CtrlEnterForNewLine);
            ImGui::InputTextMultiline("##source", text_input, IM_ARRAYSIZE(text_input), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * text_input_size), text_flags);

            /* TokenList checkbox */
            ImGui::Checkbox("TokenList", &checkboxval);

            ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX()+100 ,ImGui::GetCursorPosY()));

            // if (ImGui::Button("Quit")){
            //     glfwSetWindowShouldClose(window, true);
            // }

            /* Get filepath from windows file dialog */
            if (ImGui::Button("Browse"))
            {
                file_path = Windows_file_dialog::OpenFile("All Files (*.*)\0*.*\0", window);
                if(!file_path.empty()){
                    std::replace(file_path.begin(), file_path.end(), '\\', '/');  // Windows path stuff
                    std::cout << file_path.c_str() << std::endl;
                    MyApp::loadfile(file_path.c_str(),text_input);                // Put file contents in the textbox
                    // system("pause");
                }else{
                    /* User cancelled */
                    strcpy(text_input, "Please Choose a file or enter data.");
                }
            }

            ImGui::SameLine(0, 20);

            /* Lexer */
            if (ImGui::Button("Lexer"))
            {
                std::string myString = removeEmptyLines(text_input);
                /* Output textbox contents to a new file */
                std::ofstream outFile("../data/input.txt");
                if (outFile.is_open()) {
                    outFile << myString;
                    outFile.close();
                    std::cout << "String saved to file successfully." << std::endl;
                } else {
                    std::cerr << "Error opening the file." << std::endl;
                }

                /* Clear table */
                identifiers.clear();
                tokens.clear();

                /* If tokenlist option is enabled just output directly else run the lexer */
                if(checkboxval){
                    MyApp::readFile("./data/input.txt", identifiers, tokens);
                }else{
                    /* Run The program and pipe stderr as well as stdout*/
                    terminal_output += MyApp::runProgramAndGetOutput(".\\build\\Main.exe .\\data\\input.txt -scanner -lexer 2>&1");
                    // system(".\\build\\Main.exe .\\data\\input.txt -scanner -lexer 2>&1");
                    // system("pause");
                    MyApp::readFile("./data/out.txt", identifiers, tokens);
                }
            }

            ImGui::SameLine(0, 20);

            /* Parser */
            if (ImGui::Button("Parser"))
            {
                std::string myString = removeEmptyLines(text_input);
                /* Output textbox contents to a new file */
                std::ofstream outFile("./data/input.txt");
                if (outFile.is_open()) {
                    outFile << myString;
                    outFile.close();
                    std::cout << "String saved to file successfully." << std::endl;
                } else {
                    std::cerr << "Error opening the file." << std::endl;
                }

                /* Use token List or lexer for tokens */
                if(checkboxval){
                    terminal_output += MyApp::runProgramAndGetOutput(".\\build\\Main.exe .\\data\\input.txt -parser -toklist 2>&1");
                    // system(".\\build\\Main.exe .\\data\\input.txt -parser -toklist 2>&1");
                    // system("pause");
                }else{
                    terminal_output += MyApp::runProgramAndGetOutput(".\\build\\Main.exe .\\data\\input.txt -parser -lexer 2>&1");
                    // system(".\\build\\Main.exe .\\data\\input.txt -parser -lexer 2>&1");
                    // system("pause");
                }
                terminal_output += MyApp::runProgramAndGetOutput(".\\Graphviz\\bin\\dot.exe -Tpng -o .\\data\\output.png .\\data\\tree.dot 2>&1");
                // system(".\\Graphviz\\bin\\dot.exe -Tpng -o .\\data\\output.png .\\data\\tree.dot 2>&1");
                // system("pause");
                // system("dot -Tpng -o .\\data\\output.png .\\data\\tree.dot");

                /* Load Output image to screen */
                loaded = img.GetImageFromPath("./data/output.png");
            }
        ImGui::End();
        }
        /* *********************************************************************** */

        if(table)
        {
        /* Table */
        static ImGuiTableFlags table_flags = ImGuiTableFlags_SizingStretchSame |
                                             ImGuiTableFlags_Resizable         |
                                             ImGuiTableFlags_BordersOuter      |
                                             ImGuiTableFlags_BordersInner      |   
                                             ImGuiTableFlags_BordersV          |
                                             ImGuiTableFlags_BordersH          |
                                             ImGuiTableFlags_ContextMenuInBody;

        static char buf[1024 * 16] = "Table";

        ImGui::Begin("Lexical Analysis", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::BeginTable("Tokens", 2, table_flags))
        {
            ImGui::TableSetupColumn("TokenLiterals");
            ImGui::TableSetupColumn("TokenType");
            ImGui::TableHeadersRow();
            for (size_t i = 0; i < identifiers.size(); ++i) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                strcpy(buf,tokens[i].c_str());
                ImGui::TextUnformatted(buf);
     
                ImGui::TableSetColumnIndex(1);
                strcpy(buf,identifiers[i].c_str());
                ImGui::TextUnformatted(buf);
            }
            ImGui::EndTable();
        }
        ImGui::End();
        }
        ImGui::Begin("Converter");
            static double amount[10]={0};
            if(ImGui::InputDouble("bits", &amount[0], 0.1, 1.0, "%g")){
                UpdateUnits(amount,0);
            }
            if(ImGui::InputDouble("bytes", &amount[1], 0.1, 1.0, "%g")){
                UpdateUnits(amount,1);
            }
            if(ImGui::InputDouble("kilobits",&amount[2], 0.1, 1.0, "%g")){
                UpdateUnits(amount,2);
            }
            if(ImGui::InputDouble("kilobytes",&amount[3], 0.1, 1.0, "%g")){
                UpdateUnits(amount,3);
            }
            if(ImGui::InputDouble("megabits", &amount[4], 0.1, 1.0, "%g")){
                UpdateUnits(amount,4);
            }
            if(ImGui::InputDouble("megabytes",&amount[5], 0.1, 1.0, "%g")){
                UpdateUnits(amount,5);
            }
            if(ImGui::InputDouble("gigabits", &amount[6], 0.1, 1.0, "%g")){
                UpdateUnits(amount,6);
            }
            if(ImGui::InputDouble("gigabytes",&amount[7], 0.1, 1.0, "%g")){
                UpdateUnits(amount,7);
            }
            if(ImGui::InputDouble("terabytes", &amount[8], 0.1, 1.0, "%g")){
                UpdateUnits(amount,8);
            }
            if(ImGui::InputDouble("petabytes", &amount[9], 0.1, 1.0, "%g")){
                UpdateUnits(amount,9);
            }
            ImGui::Separator();

            if(ImGui::InputText("DEC", bases[0], 1024)){
                ConvertBases(10);
            }
            if(ImGui::InputText("HEX", bases[1], 1024)){
                ConvertBases(16);
            }
            if(ImGui::InputText("OCT", bases[2], 1024)){
                ConvertBases(8);
            }
            if(ImGui::InputText("BIN", bases[3], 1024)){
                ConvertBases(2);
            }

        ImGui::End();

        if(image)
        {
        ImGui::Begin("Syntax tree");
            ImGui::SliderInt("Width", &img.width, 1, 1920);
            ImGui::SliderInt("Height", &img.height, 1, 1020);

            ImGui::Image((ImTextureID)img.image_texture, ImVec2(img.width, img.height));

        ImGui::End();
        }
        /* *********************************************************************** */

    }
}
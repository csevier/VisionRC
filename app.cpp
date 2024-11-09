#include "app.h"
#include <SDL2/SDL.h>
#include <SDL_mixer.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/opencv.hpp>
#include <imgui.h>
#include <string_view>
#include <imfilebrowser.h>
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "camera.h"
#include "race.h"


using namespace std;


App::App()
{
}

int App::Initialize_Subsystems()
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }
// From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    int width = 1920;
    int height = 1080;
    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm)!=0)
    {
        printf("Error: SDL_GetDesktopDisplayMode(): %s\n", SDL_GetError());
    }
    else
    {
        width = dm.w;
        height = dm.h;
    }
    // Create window with SDL_Renderer graphics context
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    mWindow = SDL_CreateWindow("VisionRC", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, window_flags);
    if (mWindow == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (mRenderer == nullptr)
    {
        SDL_Log("Error creating SDL_Renderer!");
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();
    ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(mWindow, mRenderer);
    ImGui_ImplSDLRenderer2_Init(mRenderer);


    return 1;

}

int App::Run()
{

    // Our state
    bool show_demo_window = false;
    //ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
    std::unique_ptr<Camera> race_camera = nullptr;

    // Main loop
    bool hasCheckedForSources = false;
    bool done = false;
    unsigned int a = 0;
    unsigned int b = 0;
    double delta = 0;
    double desiredFPS = 30;
    static int currentCamId = -1;
    ImGui::FileBrowser fileDialog;
    std::string  errorMessage;
     //std::map<int, std::unique_ptr<Camera>> availableSources;
    std::vector<std::unique_ptr<Camera>> availableSources;
    fileDialog.SetTitle("Offline Race Source");
    fileDialog.SetTypeFilters({ ".mp4"});
    while (!done)
    {
        a = SDL_GetTicks();
        delta = a - b;
        if (delta > 1000 / desiredFPS )
        {
            b = a;

            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT)
                    done = true;
                if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(mWindow))
                    done = true;
            }
            // if (SDL_GetWindowFlags(mWindow) & SDL_WINDOW_MINIMIZED)
            // {
            //     SDL_Delay(10);
            //     continue;
            // }

            // Start the Dear ImGui frame
            ImGui_ImplSDLRenderer2_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();
            if (race_camera != nullptr)
            {
                desiredFPS = race_camera->GetCameraFPS();
                race_camera->NextFrame();
                CURRENT_RACE.Update(*race_camera);
                race_camera->Draw();
                if(CURRENT_RACE.Draw())
                {
                    if(currentCamId != -1)
                    {
                        availableSources.clear();
                        currentCamId = -1;
                        hasCheckedForSources = false;
                    }
                    race_camera = nullptr;
                }
                ImGui::Begin("Timing Fidelity");
                std::string label="Camera Polling Every " + std::to_string(1000.0f / ImGui::GetIO().Framerate) +  " ms or " + std::to_string((int)ImGui::GetIO().Framerate) + " times per second";
                if(ImGui::GetIO().Framerate < 25)
                {
                    ImGui::TextColored(ImVec4(1,0,0,1),label.c_str());
                }
                else
                {
                    ImGui::TextColored(ImVec4(0,1,0,1),label.c_str());
                }
                ImGui::End();
            }
            else
            {
                ImGui::Begin("Select Race Camera Source");
                if (!availableSources.empty())
                {
                    errorMessage.clear();
                    std::string sourcesFromVec;
                    for(int i = 0; i < availableSources.size(); i ++)
                    {
                         sourcesFromVec += std::to_string(i) +'\0';
                    }
                    if (ImGui::Combo("Live From USB Source", &currentCamId, sourcesFromVec.c_str()))
                    {
                        race_camera = std::move(availableSources[currentCamId]);
                    }
                }
                else
                {
                    if(!hasCheckedForSources)
                    {
                        for (int i = 0; i < 15; i++)
                        {
                            try
                            {
                                std::unique_ptr<Camera> cam = std::make_unique<Camera>(mRenderer, i);
                                availableSources.push_back(std::move(cam));
                                errorMessage.clear();
                            }
                            catch(std::exception ex)
                            {
                                errorMessage = "No usb camera found, is one plugged in and not already in use?";
                            }
                        }
                        hasCheckedForSources = true;
                    }
                }
                if(ImGui::Button("Live From IP Camera"))
                {
                    ImGui::OpenPopup("IpCam");

                }

                if (ImGui::BeginPopupModal("IpCam"))
                {
                    ImGui::Text("Please put in a rtsp url.");
                    ImGui::Separator();

                    static char rtspUrl[255] = "";
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                    ImGui::InputText("URL", rtspUrl, IM_ARRAYSIZE(rtspUrl));
                    ImGui::PopStyleVar();

                    if (ImGui::Button("OK", ImVec2(120, 0)))
                    {
                        try
                        {
                            race_camera = std::make_unique<Camera>(mRenderer, rtspUrl, false);
                            memset(rtspUrl, 0, sizeof rtspUrl);
                            errorMessage.clear();
                            ImGui::CloseCurrentPopup();
                        }
                        catch(std::exception ex)
                        {
                            errorMessage = "No ip camera found, are you sure url is correct?";
                        }
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(120, 0)))
                    {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                ImGui::SameLine();
                if(ImGui::Button("Offline From Video"))
                {
                    fileDialog.Open();
                }
                if(!errorMessage.empty())
                {
                    ImGui::TextColored(ImVec4(1,0,0,1), errorMessage.c_str());
                }
                ImGui::End();
                // select cam type
            }
            fileDialog.Display();
            if(fileDialog.HasSelected())
            {
                try
                {
                    race_camera = std::make_unique<Camera>(mRenderer, fileDialog.GetSelected().string(), true);
                    errorMessage.clear();
                }
                catch(std::exception ex)
                {
                    errorMessage = "Could not open file. Are you sure its valid mp4?";
                }

                fileDialog.ClearSelected();
            }
            // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
            if (show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);

            // Rendering
            ImGui::Render();
            SDL_RenderSetScale(mRenderer, ImGui::GetIO().DisplayFramebufferScale.x, ImGui::GetIO().DisplayFramebufferScale.y);
            SDL_SetRenderDrawColor(mRenderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
            SDL_RenderClear(mRenderer);
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), mRenderer);
            SDL_RenderPresent(mRenderer);
        }
    }

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(mRenderer);
    SDL_DestroyWindow(mWindow);
    SDL_Quit();

    return 0;
}

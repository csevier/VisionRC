#include "app.h"
#include <SDL2/SDL.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/opencv.hpp>
#include <imgui.h>
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
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

// From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with SDL_Renderer graphics context
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    mWindow = SDL_CreateWindow("VisionRC", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, window_flags);
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
    //SDL_RendererInfo info;
    //SDL_GetRendererInfo(renderer, &info);
    //SDL_Log("Current SDL_Renderer: %s", info.name);

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

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);


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
    bool done = false;
    unsigned int a = 0;
    unsigned int b = 0;
    double delta = 0;
    double desiredFPS = 30;
    ImGui::FileBrowser fileDialog;
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
                CURRENT_RACE.Draw();
                ImGui::Begin("Performance Counter");                          // Create a window called "Hello, world!" and append into it.
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::End();
            }
            else
            {
                static char str0[255] = "";
                static std::string message;
                ImGui::Begin("Select Race Camera Source");
                if(ImGui::Button("Live From USB Camera"))
                {
                    race_camera = std::make_unique<Camera>(mRenderer, 0);
                }
                ImGui::SameLine();
                if(ImGui::Button("Live From IP Camera"))
                {
                    //race_camera = std::make_unique<Camera>(mRenderer, "rtsp://<ip>:port/etc");
                }
                ImGui::SameLine();
                if(ImGui::Button("Offline Video"))
                {
                    fileDialog.Open();
                }
                ImGui::End();
                // select cam type
            }
            fileDialog.Display();
            if(fileDialog.HasSelected())
            {
                race_camera = std::make_unique<Camera>(mRenderer, fileDialog.GetSelected().string());
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

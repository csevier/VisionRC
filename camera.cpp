#include "camera.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "imgui.h"
#include "race.h"


Camera::Camera(SDL_Renderer* renderer, int id)
{
    mRenderer = renderer;
    mVideo = cv::VideoCapture(id, cv::CAP_V4L2);
    mVideo.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    mVideo.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    mVideo.set(cv::CAP_PROP_BRIGHTNESS, 128);
    mVideo.set(cv::CAP_PROP_AUTO_EXPOSURE, 1);
    mMasks["main_hsv"] =  std::make_unique<CameraFrame>(renderer);
    mMasks["main_bgr"] =  std::make_unique<CameraFrame>(renderer);
    mMasks["select_color"] =  std::make_unique<CameraFrame>(renderer);
}

void Camera::NextFrame()
{
    cv::Mat currentFrameBGR;
    cv::Mat currentFrameHSV;
    mVideo >> currentFrameBGR;;
    cv::cvtColor(currentFrameBGR, currentFrameHSV,cv::COLOR_BGR2HSV_FULL);
    mFrameTimeStamp = std::chrono::system_clock::now();
    mMasks["main_bgr"]->SetMatrix(currentFrameBGR);
    mMasks["main_hsv"]->SetMatrix(currentFrameHSV);
    UpdateColorSelectFrame();
}

std::chrono::time_point<std::chrono::system_clock>& Camera::GetFrameTimeStamp()
{
    return mFrameTimeStamp;
}

bool Camera::RacerInFrame(Racer& racer)
{
    if(!mMasks.count(racer.GetName()))
    {
       mMasks[racer.GetName()] = std::make_unique<CameraFrame>(mRenderer);
    }
    cv::Scalar upperColor = cv::Scalar(racer.GetUpperColorHSV255().x,racer.GetUpperColorHSV255().y,racer.GetUpperColorHSV255().z);
    cv::Scalar lowerColor = cv::Scalar(racer.GetLowerColorHSV255().x,racer.GetLowerColorHSV255().y,racer.GetLowerColorHSV255().z);

    cv::Mat racerMatrix;
    cv::inRange( mMasks["main_hsv"]->GetMatrix(),lowerColor,upperColor, racerMatrix);
    mMasks[racer.GetName()]->SetMatrix(racerMatrix);


    // todo will be racers matrix;
    int resultCount = cv::countNonZero(racerMatrix);
    cv::Mat out;
    cv::cvtColor(mMasks[racer.GetName()]->GetMatrix(), out, cv::COLOR_GRAY2BGR);
    cv::bitwise_and(mMasks["main_bgr"]->GetMatrix(),out, out);
    mMasks[racer.GetName()]->SetMatrix(out);
    mRacerFoundThisFrame = resultCount >0;
    return mRacerFoundThisFrame;
}

Camera::~Camera()
{
    mVideo.release();
}


void Camera::Draw()
{
    ImVec2 uv_min = ImVec2(0.0f, 0.0f);
    ImVec2 uv_max = ImVec2(1.0f, 1.0f);
    ImGui::Begin("Race Camera");
    ImGui::Image((void*)mMasks["main_bgr"]->GetTexture(), ImVec2(mMasks["main_bgr"]->GetWidth(),mMasks["main_bgr"]->GetHeight()),uv_min,uv_max);
    ImVec2 race_cam_min_loc = ImGui::GetItemRectMin();
    ImVec2 race_cam_size = ImGui::GetItemRectSize();
    if (ImGui::BeginPopupContextWindow("my popup"))
    {
        ImGui::SeparatorText("Racers!");
        for (auto& racer : CURRENT_RACE.GetRacers())
        {
            if (ImGui::Selectable(racer.first.c_str(), ImGuiSelectableFlags_DontClosePopups))
            {
                racer.second.SetColor(mRacerColorFromMouseHSV255.x,mRacerColorFromMouseHSV255.y, mRacerColorFromMouseHSV255.z);
            }
        }
        ImGui::EndPopup();
    }
    else
    {
         SampleColor(race_cam_min_loc, race_cam_size);
    }
    ImGui::End();
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    ImGui::Begin("Masks");
    if (ImGui::BeginTabBar("Masks", tab_bar_flags))
    {
        for (const auto& mask : mMasks )
        {
            if (ImGui::BeginTabItem(mask.first.c_str()))
            {
                ImGui::Image((void*)mask.second->GetTexture(), ImVec2(mask.second->GetWidth(),mask.second->GetHeight()));
                ImGui::EndTabItem();
            }
        }

        ImGui::EndTabBar();
    }

   ImGui::End();
}

void Camera::SampleColor(ImVec2 race_cam_min_loc, ImVec2 race_cam_size)
{
    ImVec2 mouse_pos = ImGui::GetMousePos();
    float mouse_in_cam_pos_x = mouse_pos.x - race_cam_min_loc.x;
    float mouse_in_cam_pos_y = mouse_pos.y - race_cam_min_loc.y;
    bool isMouseInCam = mouse_in_cam_pos_x > 0 && mouse_in_cam_pos_x < race_cam_size.x  &&
                        mouse_in_cam_pos_y > 0 && mouse_in_cam_pos_y < race_cam_size.y;
    if(isMouseInCam)
    {
        cv::Vec3b colorUnderMouse = mMasks["main_hsv"]->GetMatrix().at<cv::Vec3b>(cv::Point((int)mouse_in_cam_pos_x ,(int)mouse_in_cam_pos_y));
        mRacerColorFromMouseHSV255.w =255;
        mRacerColorFromMouseHSV255.x = colorUnderMouse[0];
        mRacerColorFromMouseHSV255.y = colorUnderMouse[1];
        mRacerColorFromMouseHSV255.z = colorUnderMouse[2];
    }
}

void Camera::UpdateColorSelectFrame()
{
    cv::Scalar colorAtCursor = cv::Scalar(mRacerColorFromMouseHSV255.x,mRacerColorFromMouseHSV255.y,mRacerColorFromMouseHSV255.z);
    cv::inRange( mMasks["main_hsv"]->GetMatrix(),colorAtCursor,colorAtCursor, mMasks["select_color"]->GetMatrix());
    cv::Mat out;
    cv::cvtColor(mMasks["select_color"]->GetMatrix(), out, cv::COLOR_GRAY2BGR);
    cv::bitwise_and( mMasks["main_bgr"]->GetMatrix(),out, out);
    mMasks["select_color"]->SetMatrix(out);
}
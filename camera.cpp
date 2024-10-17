#include "camera.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "imgui.h"
#include "race.h"

Camera::Camera(SDL_Renderer* renderer, std::string filenameOrIp)
{
    mRenderer = renderer;
    mVideo = cv::VideoCapture(filenameOrIp);
    mVideo.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    mVideo.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    mVideo.set(cv::CAP_PROP_BRIGHTNESS, 128);
    mVideo.set(cv::CAP_PROP_AUTO_EXPOSURE, 1);
    mFrameCount = mVideo.get(cv::CAP_PROP_FRAME_COUNT);
    mCameraFPS = mVideo.get(cv::CAP_PROP_FPS);
    mMasks["main_hsv"] =  std::make_unique<CameraFrame>(renderer);
    mMasks["main_bgr"] =  std::make_unique<CameraFrame>(renderer);
    mMasks["select_color"] =  std::make_unique<CameraFrame>(renderer);
    mIsOfflineMode = true;
}

Camera::Camera(SDL_Renderer* renderer, int id)
{
    mRenderer = renderer;
    mVideo = cv::VideoCapture(id, cv::CAP_V4L2);
    mVideo.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    mVideo.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    mVideo.set(cv::CAP_PROP_BRIGHTNESS, 128);
    mVideo.set(cv::CAP_PROP_AUTO_EXPOSURE, 1);
    mExposure = mVideo.get(cv::CAP_PROP_EXPOSURE);
    mBrightness = mVideo.get(cv::CAP_PROP_BRIGHTNESS);
    mCameraFPS = mVideo.get(cv::CAP_PROP_FPS);
    mMasks["main_hsv"] =  std::make_unique<CameraFrame>(renderer);
    mMasks["main_bgr"] =  std::make_unique<CameraFrame>(renderer);
    mMasks["select_color"] =  std::make_unique<CameraFrame>(renderer);
    mIsOfflineMode = false;
}

void Camera::NextFrame()
{
    if (mIsOfflineMode && mPause )
    {
        UpdateColorSelectFrame();
        return;
    }
    if (mIsOfflineMode && mCurrentFrame == mFrameCount)
    {
        UpdateColorSelectFrame();
        return;
    }
    cv::Mat currentFrameBGR;
    cv::Mat currentFrameHSV;
    mVideo >> currentFrameBGR;;
    cv::cvtColor(currentFrameBGR, currentFrameHSV,cv::COLOR_BGR2HSV_FULL);
    mFrameTimeStamp = std::chrono::system_clock::now();
    mMasks["main_bgr"]->SetMatrix(currentFrameBGR);
    mMasks["main_hsv"]->SetMatrix(currentFrameHSV);
    UpdateColorSelectFrame();
    if(mRecord && mVideoOut.isOpened())
    {
       mVideoOut.write(currentFrameBGR);
    }
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
    mVideoOut.release();
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
        ImGui::SeparatorText("Apply Color To:");
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
    if (!mIsOfflineMode)
    {
        if(ImGui::SliderInt("Exposure", &mExposure, 0, 1000))
        {
            mVideo.set(cv::CAP_PROP_EXPOSURE, mExposure);
        }
        if (ImGui::SliderInt("Brightness", &mBrightness, 0, 1000))
        {
            mVideo.set(cv::CAP_PROP_BRIGHTNESS, mBrightness);
        }
    }
    else
    {
        if(ImGui::Button("Last Frame"))
        {
            mVideo.set(cv::CAP_PROP_POS_FRAMES, mCurrentFrame -2);
            mPause = false;
            NextFrame();
            mPause = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("Pause"))
        {
            mPause = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("Play"))
        {
            mPause = false;
        }
        ImGui::SameLine();
        if(ImGui::Button("Next Frame"))
        {
            mPause = false;
            NextFrame();
            mPause = true;
        }
        if (ImGui::SliderInt("Time", &mCurrentFrame, 0, mFrameCount, FormatTime(FrameAsTime(mCurrentFrame)).c_str()))
        {
            mPause = false;
            mVideo.set(cv::CAP_PROP_POS_FRAMES, mCurrentFrame -2);
            NextFrame();
            mPause = true;
        }

        mCurrentFrame = (int)mVideo.get(cv::CAP_PROP_POS_FRAMES);
        std::string frame_pos_label = FormatTime(FrameAsTime(mCurrentFrame)) + " / " + FormatTime(FrameAsTime(mFrameCount));
        ImGui::SameLine();
        ImGui::Text(frame_pos_label.c_str());
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

void Camera::Record(std::string& location)
{
    if(!mVideoOut.isOpened())
    {
        int codec = cv::VideoWriter::fourcc('m','p', '4', 'v');
        mVideoOut = cv::VideoWriter(location,codec,30, cv::Size2i(640,480));
    }

    mRecord = true;
}

void Camera::StopRecording()
{
    mVideoOut.release();
    mRecord = false;
}

double Camera::FrameAsTime(int frame)
{
    return frame / mCameraFPS;
}

std::string Camera::FormatTime(double time)
{
    std::chrono::milliseconds ms((int)time *1000);
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(ms);
    ms -= std::chrono::duration_cast<std::chrono::milliseconds>(secs);
    auto mins = std::chrono::duration_cast<std::chrono::minutes>(secs);
    secs -= std::chrono::duration_cast<std::chrono::seconds>(mins);
    auto hour = std::chrono::duration_cast<std::chrono::hours>(mins);
    mins -= std::chrono::duration_cast<std::chrono::minutes>(hour);
    std::stringstream out;
    out << mins.count() << ":" << secs.count() << ":" << ms.count();
    return out.str();
}

double Camera::GetCameraFPS()
{
    return mCameraFPS;
}

void Camera::Pause()
{
    mPause = true;
}

void Camera::Unpause()
{
    mPause = false;
}

bool Camera::IsVideoOver()
{
    if (mIsOfflineMode)
    {
        return mCurrentFrame >= mFrameCount;
    }
    else
    {
        return false;
    }

}

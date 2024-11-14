#include "camera.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "imgui.h"
#include "race.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <ostream>

Camera::Camera(SDL_Renderer* renderer, std::string filenameOrIp, bool isOffline)
{
    mZoneExportDialogue.SetTitle("Exporting Zones");
    mZoneExportDialogue.SetTypeFilters({ ".txt"});
    mZoneImportDialogue.SetTitle("Importing Zones");
    mZoneImportDialogue.SetTypeFilters({ ".txt"});
    mRenderer = renderer;
    mVideo = cv::VideoCapture(filenameOrIp);
    if(!mVideo.isOpened())
    {
        throw std::exception();
    }
    mVideo.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    mVideo.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    mVideo.set(cv::CAP_PROP_BRIGHTNESS, 128);
    mVideo.set(cv::CAP_PROP_AUTO_EXPOSURE, 1);
    mFrameCount = mVideo.get(cv::CAP_PROP_FRAME_COUNT);
    mCameraFPS = mVideo.get(cv::CAP_PROP_FPS);
    mVideo.set(cv::CAP_PROP_FPS, 60);
    mCameraFPS = mVideo.get(cv::CAP_PROP_FPS);
    mMasks["main_hsv"] =  std::make_unique<CameraFrame>(renderer);
    mMasks["main_bgr"] =  std::make_unique<CameraFrame>(renderer);
    mMasks["select_color_filtered"] =  std::make_unique<CameraFrame>(renderer);
    mMasks["select_color"] =  std::make_unique<CameraFrame>(renderer);
    mIsOfflineMode = isOffline;
}

Camera::Camera(SDL_Renderer* renderer, int id)
{
    mZoneExportDialogue.SetTitle("Exporting Zones");
    mZoneExportDialogue.SetTypeFilters({ ".txt"});
    mZoneImportDialogue.SetTitle("Importing Zones");
    mZoneImportDialogue.SetTypeFilters({ ".txt"});

    mRenderer = renderer;
    #ifdef __linux__
        mVideo = cv::VideoCapture(id, cv::CAP_V4L2);
    #elif _WIN32
        mVideo = cv::VideoCapture(id);
    #elif __APPLE__
        mVideo = cv::VideoCapture(id);
    #endif


    if(!mVideo.isOpened())
    {
        throw std::exception();
    }
    mVideo.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    mVideo.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    mVideo.set(cv::CAP_PROP_BRIGHTNESS, 128);
    mVideo.set(cv::CAP_PROP_AUTO_EXPOSURE, 1);
    mExposure = mVideo.get(cv::CAP_PROP_EXPOSURE);
    mBrightness = mVideo.get(cv::CAP_PROP_BRIGHTNESS);
    mCameraFPS = mVideo.get(cv::CAP_PROP_FPS);
    mVideo.set(cv::CAP_PROP_FPS, 60);
    mCameraFPS = mVideo.get(cv::CAP_PROP_FPS);
    mMasks["main_hsv"] =  std::make_unique<CameraFrame>(renderer);
    mMasks["main_bgr"] =  std::make_unique<CameraFrame>(renderer);
    mMasks["select_color_filtered"] =  std::make_unique<CameraFrame>(renderer);
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
    mVideo >> currentFrameBGR;
    if (currentFrameBGR.size().width != 640 || currentFrameBGR.size().height !=480)
    {
        cv::resize(currentFrameBGR, currentFrameBGR, cv::Size(640, 480), 0, 0, cv::INTER_CUBIC);
    }
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
    mRacerFoundThisFrame = false;
    racer.inFrame = false;
    racer.mCurrentPixels = 0;
    if(!mMasks.count(racer.GetName()))
    {
       mMasks[racer.GetName()] = std::make_unique<CameraFrame>(mRenderer);
    }
    cv::Scalar upperColor = cv::Scalar(racer.GetUpperColorHSV255().x,racer.GetUpperColorHSV255().y,racer.GetUpperColorHSV255().z);
    cv::Scalar lowerColor = cv::Scalar(racer.GetLowerColorHSV255().x,racer.GetLowerColorHSV255().y,racer.GetLowerColorHSV255().z);
    cv::Mat racerMatrix;
    cv::inRange( mMasks["main_hsv"]->GetMatrix(),lowerColor,upperColor, racerMatrix);
    mMasks[racer.GetName()]->SetMatrix(racerMatrix);
    int fullFrameResultCount = cv::countNonZero(racerMatrix);
    cv::Mat out;
    cv::cvtColor(mMasks[racer.GetName()]->GetMatrix(), out, cv::COLOR_GRAY2BGR);
    cv::bitwise_and(mMasks["main_bgr"]->GetMatrix(),out, out);
    mMasks[racer.GetName()]->SetMatrix(out);

    if (!polyZones.empty())
    {
        for (int i = 0; i < polyZones.size(); i++)
        {
            cv::Mat poly_zone_mask= cv::Mat::zeros(mMasks["main_hsv"]->GetMatrix().rows,mMasks["main_hsv"]->GetMatrix().cols, CV_8UC1);
            std::vector<std::vector<cv::Point>> cvPointsAll;
            std::vector<cv::Point> cvPoly;
            for(ImVec2 p : polyZones[i])
            {
                cvPoly.push_back(cv::Point(p.x, p.y));
            }
            cvPointsAll.push_back(cvPoly);
            cv::fillPoly(poly_zone_mask, cvPointsAll, cv::Scalar(255));
            cv::Mat poly_zone_matrix;
            mMasks["main_hsv"]->GetMatrix().copyTo(poly_zone_matrix, poly_zone_mask);
            cv::Mat racerMatrix;
            cv::inRange(poly_zone_matrix,lowerColor,upperColor, racerMatrix);
            int resultCount = cv::countNonZero(racerMatrix);
            racer.mCurrentPixels = resultCount;
            bool racerInZone = resultCount > racer.mRequiredPixels;
            if (racerInZone && i ==0) // finish line is always zone 0
            {
                mRacerFoundThisFrame = true;
                racer.inFrame = true;
                if (racer.mCurrentZone !=-1)
                {
                    racer.mLastZone = racer.mCurrentZone;
                }
                racer.mCurrentZone = i;
                return mRacerFoundThisFrame; // just return first zone
            }
            else if (racerInZone)
            {
                if (racer.mCurrentZone !=-1)
                {
                    racer.mLastZone = racer.mCurrentZone;
                }
                racer.mCurrentZone = i;
                return mRacerFoundThisFrame; // just return first zone
            }
        }
        racer.mCurrentZone = -1;
    }
    else
    {
        mRacerFoundThisFrame = fullFrameResultCount > racer.mRequiredPixels;
        racer.mCurrentPixels = fullFrameResultCount;
        if (mRacerFoundThisFrame)
        {
            racer.inFrame = true;
        }
        else
        {
            racer.inFrame = false;
        }
    }
    return mRacerFoundThisFrame;
}

Camera::~Camera()
{
    mVideo.release();
    mVideoOut.release();
}


void Camera::Draw()
{
    if(ImGui::BeginMainMenuBar())
    {
        if(ImGui::BeginMenu("Zones"))
        {
            if(ImGui::MenuItem("Import Zones", NULL, false, CURRENT_RACE.GetRaceStatus() == RaceStatus::NOT_STARTED))
            {
                mZoneImportDialogue.Open();
            }
            if(ImGui::MenuItem("Export Zones", NULL, false, CURRENT_RACE.GetRaceStatus() == RaceStatus::NOT_STARTED))
            {
                mZoneExportDialogue.Open();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    ImGui::Begin("Race Camera");
    ImGui::BeginTabBar("Race Camera");
    ImGui::BeginTabItem("Race Camera");
    ImGui::Image((void*)mMasks["main_bgr"]->GetTexture(), ImVec2(640,480));
    ImGui::EndTabItem();
    ImGui::EndTabBar();

    ImVec2 race_cam_min_loc = ImGui::GetItemRectMin();
    ImVec2 race_cam_size = ImGui::GetItemRectSize();
    ImVec2 mouse_pos  = ImGui::GetMousePos();
    float mouse_in_cam_pos_x = mouse_pos.x - race_cam_min_loc.x;
    float mouse_in_cam_pos_y = mouse_pos.y - race_cam_min_loc.y;
    static std::vector<ImVec2> zonePolygon;
    static bool isDrawingPolyZone = false;
    if (ImGui::IsItemClicked(2)) // polygon zone.
    {
        auto mouse = ImVec2(mouse_in_cam_pos_x, mouse_in_cam_pos_y);
        if (zonePolygon.size() > 0)
        {
            ImVec2 firstPoint = zonePolygon[0];
            double distance = sqrt(pow((mouse.x - firstPoint.x),2) + pow((mouse.y - firstPoint.y),2));
            if (abs(distance) <10) // 10 pixels
            {
                polyZones.push_back(zonePolygon);
                zonePolygon.clear();
                isDrawingPolyZone = false;
            }
            else
            {
                zonePolygon.push_back(ImVec2(mouse_in_cam_pos_x, mouse_in_cam_pos_y));
                isDrawingPolyZone = true;
            }
        }
        else
        {
            zonePolygon.push_back(ImVec2(mouse_in_cam_pos_x, mouse_in_cam_pos_y));
            isDrawingPolyZone = true;
        }
    }
    if(isDrawingPolyZone)
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        if (!zonePolygon.empty()) draw_list->AddCircle(RaceCamToMouseCoords(race_cam_min_loc,zonePolygon[0]), 10.0f,IM_COL32_WHITE);
        for (ImVec2 p : zonePolygon)
        {
            draw_list->PathLineTo(RaceCamToMouseCoords(race_cam_min_loc,p));
        }
        draw_list->PathLineTo(mouse_pos);
        draw_list->PathStroke(IM_COL32_WHITE, ImDrawFlags_None);
    }
    // and then draw the finished ones filled?.
    for (int i =0; i < polyZones.size(); i++)
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        if (i ==0)
        {
            draw_list->AddText(RaceCamToMouseCoords(race_cam_min_loc,polyZones[i][0]),IM_COL32_WHITE, "Start/Finish");
        }
        else
        {
            draw_list->AddText(RaceCamToMouseCoords(race_cam_min_loc,polyZones[i][0]),IM_COL32_WHITE, std::to_string(i).c_str());
        }
        auto pos = ImGui::GetCursorScreenPos();
        if (CURRENT_RACE.GetRaceStatus() != RaceStatus::RUNNING)
        {
            // Get the absolute position where you want to place the button
            ImVec2 coord = RaceCamToMouseCoords(race_cam_min_loc,polyZones[i][0]);
            ImVec2 buttonPos = ImVec2(coord.x-20, coord.y);// Example position

            // Set the cursor position to the desired location
            ImGui::SetCursorScreenPos(buttonPos);

            // Create the button
            if (ImGui::Button(("x##" + std::to_string(i)).c_str()))
            {
                polyZones.erase(polyZones.begin() +i);
            }
            ImGui::SetCursorScreenPos(pos);
        }
        for (ImVec2 p : polyZones[i])
        {
            draw_list->PathLineTo(RaceCamToMouseCoords(race_cam_min_loc,p));
        }
        draw_list->PathStroke(IM_COL32_WHITE, ImDrawFlags_Closed);
    }

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
        ImGui::Checkbox("Auto Exposure - Can effect race times in low light settings.", &mAutoExposure);
        if (mAutoExposure)
        {
            mVideo.set(cv::CAP_PROP_AUTO_EXPOSURE, 3);
        }
        else
        {
            mVideo.set(cv::CAP_PROP_AUTO_EXPOSURE, 1);
            if(ImGui::SliderInt("Exposure", &mExposure, 0, 1000))
            {
                mVideo.set(cv::CAP_PROP_EXPOSURE, mExposure);
            }
            if (ImGui::SliderInt("Brightness", &mBrightness, 0, 1000))
            {
                mVideo.set(cv::CAP_PROP_BRIGHTNESS, mBrightness);
            }
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
        ImGui::SameLine();
        if(ImGui::Button("Mark"))
        {
            if (markOne ==0)
            {
                markOne = FrameAsTime(mCurrentFrame);
            }
            else
            {
                markTwo = FrameAsTime(mCurrentFrame);
            }
        }
        ImGui::SameLine();
        if(ImGui::Button("Clear Marks"))
        {
            markOne = 0;
            markTwo = 0;
        }
        ImGui::SameLine();
        std::string frame_mark_label = FormatTime(markOne) + " - " + FormatTime(markTwo);
        ImGui::Text("%s", frame_mark_label.c_str());
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
        ImGui::Text("%s", frame_pos_label.c_str());
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
                ImGui::Image((void*)mask.second->GetTexture(), ImVec2(640,480));
                ImGui::EndTabItem();
            }
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
    if(mZoneExportDialogue.HasSelected())
    {
        std::string  exportLocation = mZoneExportDialogue.GetSelected().string();
        std::filesystem::path file = exportLocation;
        if (!file.has_extension() || file.extension() != ".txt")
        {
            exportLocation += ".txt";
        }
        ExportZones(exportLocation);
        mZoneExportDialogue.ClearSelected();
    }
    if(mZoneImportDialogue.HasSelected())
    {
        std::string  importLocation = mZoneImportDialogue.GetSelected().string();
        ImportZones(importLocation);
        mZoneImportDialogue.ClearSelected();
    }
    mZoneExportDialogue.Display();
    mZoneImportDialogue.Display();
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
ImVec2 Camera::MouseToRaceCamCoords(ImVec2 race_cam_min_loc, ImVec2 mouse)
{
    float mouse_in_cam_pos_x = mouse.x - race_cam_min_loc.x;
    float mouse_in_cam_pos_y = mouse.y - race_cam_min_loc.y;
    return ImVec2(mouse_in_cam_pos_x, mouse_in_cam_pos_y);
}

ImVec2 Camera::RaceCamToMouseCoords(ImVec2 race_cam_min_loc, ImVec2 raceCam)
{
    float mouse_pos_x = raceCam.x + race_cam_min_loc.x;
    float mouse_pos_y = raceCam.y + race_cam_min_loc.y;
    return ImVec2(mouse_pos_x, mouse_pos_y);
}

void Camera::UpdateColorSelectFrame()
{
    cv::Scalar colorAtCursor = cv::Scalar(mRacerColorFromMouseHSV255.x,mRacerColorFromMouseHSV255.y,mRacerColorFromMouseHSV255.z);
    cv::inRange( mMasks["main_hsv"]->GetMatrix(),colorAtCursor,colorAtCursor, mMasks["select_color_filtered"]->GetMatrix());
    cv::Mat out;
    cv::cvtColor(mMasks["select_color_filtered"]->GetMatrix(), out, cv::COLOR_GRAY2BGR);
    cv::bitwise_and( mMasks["main_bgr"]->GetMatrix(),out, out);
    mMasks["select_color_filtered"]->SetMatrix(out);
    cv::Mat colorSelect;
    mMasks["main_hsv"]->GetMatrix().copyTo(colorSelect); // sets correct size and params
    colorSelect.setTo(colorAtCursor);
    cv::cvtColor(colorSelect, out, cv::COLOR_HSV2BGR_FULL);
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
    std::chrono::milliseconds ms(int(time *1000));
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(ms);
    ms -= std::chrono::duration_cast<std::chrono::milliseconds>(secs);
    auto mins = std::chrono::duration_cast<std::chrono::minutes>(secs);
    secs -= std::chrono::duration_cast<std::chrono::seconds>(mins);
    auto hour = std::chrono::duration_cast<std::chrono::hours>(mins);
    mins -= std::chrono::duration_cast<std::chrono::minutes>(hour);
    std::stringstream out;
    if (mins.count() < 10)
    {
        out << "0" << mins.count();
    }
    else
    {
        out << mins.count();
    }
    out << ":";
    if (secs.count() < 10)
    {
        out << "0" << secs.count();
    }
    else
    {
        out << secs.count();
    }
    out << ":";
    if (ms.count() < 10)
    {
        out << "00" << ms.count();
    }
    else if (ms.count() < 100)
    {
        out << "0" << ms.count();
    }
    else
    {
        out << ms.count();
    }
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

void Camera::RemoveMask(std::string maskName)
{
    mMasks.erase(maskName);
}

void Camera::ExportZones(std::string fileName)
{
    std::ofstream out(fileName);
    for(auto& zone: polyZones)
    {
        for (auto& point : zone)
        {
            out << point.x << " ";
            out << point.y << " ";
        }
        out << std::endl;
    }
    out.close();
}

void Camera::ImportZones(std::string fileName)
{
    polyZones.clear();
    std::ifstream in(fileName);
    std::string points;
    while(getline(in, points))
    {
        std::stringstream pss(points);
        std::string point;
        std::vector<ImVec2> zone;
        int x = -1;
        int y = -1;
        while(pss >> point)
        {
            if (x == -1)
            {
                x = std::stoi(point);
            }
            else
            {
                y = std::stoi(point);
            }
            if (x !=-1 && y !=-1)
            {
                zone.push_back(ImVec2(x,y));
                x = -1;
                y = -1;
            }
        };
        polyZones.push_back(zone);
    }
    in.close();
}

#ifndef CAMERA_H
#define CAMERA_H
#include <opencv2/videoio.hpp>
#include "racer.h"
#include "cameraframe.h"
#include <chrono>
#include <SDL2/SDL.h>
#include <map>

class Camera
{
public:
    Camera(SDL_Renderer* renderer, int id = 0);
    Camera(SDL_Renderer* renderer, std::string filenameOrIp, bool isOffline);
    ~Camera();
    void NextFrame();
    std::chrono::time_point<std::chrono::system_clock>& GetFrameTimeStamp();
    bool RacerInFrame(Racer& racer);
    void Draw();
    void Record(std::string& location);
    void StopRecording();
    double GetCameraFPS();
    void Pause();
    void Unpause();
    bool IsVideoOver();
    void RemoveMask(std::string maskName);
    bool mAutoExposure = false;
    std::vector<std::vector<ImVec2>> polyZones;
    ImVec2 mouse_start{-1,-1};
    ImVec2 zone_start{-1,-1};
    ImVec2 mouse_end;
    ImVec2 zone_end;
    bool mIsOfflineMode = false;
    double markOne = 0;
    double markTwo = 0;

private:
    void SampleColor(ImVec2 race_cam_min_loc, ImVec2 race_cam_max_loc);
    void DrawZone(ImVec2 mouse_start, ImVec2 mouse_end);
    void UpdateColorSelectFrame();
    double FrameAsTime(int frame);
    std::string FormatTime(double time);
    cv::VideoCapture mVideo;
    cv::VideoWriter mVideoOut;
    bool mRecord = false;
    std::map<std::string, std::unique_ptr<CameraFrame>> mMasks;
    SDL_Renderer* mRenderer;
    bool mRacerFoundThisFrame = false;
    ImVec4 mRacerColorFromMouseHSV255;
    std::chrono::time_point<std::chrono::system_clock> mFrameTimeStamp;
    bool mUserIsAssigningRacerColor = false;
    int mExposure = 0;
    int mBrightness =0;
    bool mPause = false;
    int mFrameCount = 0;
    int mCurrentFrame = 0;
    double mCameraFPS = 0;
    ImVec2 MouseToRaceCamCoords(ImVec2 race_cam_min_loc,ImVec2 mouse);
    ImVec2 RaceCamToMouseCoords(ImVec2 race_cam_min_loc,ImVec2 raceCam);
};

#endif // CAMERA_H

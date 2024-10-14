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
    ~Camera();
    void NextFrame();
    std::chrono::time_point<std::chrono::system_clock>& GetFrameTimeStamp();
    bool RacerInFrame(Racer& racer);
    void Draw();
    void Record();
    void StopRecording();

private:
    void SampleColor(ImVec2 race_cam_min_loc, ImVec2 race_cam_max_loc);
    void UpdateColorSelectFrame();
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
};

#endif // CAMERA_H

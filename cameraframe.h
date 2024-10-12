#ifndef CAMERAFRAME_H
#define CAMERAFRAME_H

#include <SDL2/SDL.h>
#include <opencv2/videoio.hpp>

class CameraFrame
{
public:
    CameraFrame();
    CameraFrame(SDL_Renderer* renderer);
    void SetMatrix(cv::Mat mat);
    SDL_Texture* GetTexture();
    cv::Mat& GetMatrix();
    int GetWidth();
    int GetHeight();

private:
    SDL_Renderer* mRenderer = nullptr;
    cv::Mat mMat;
    SDL_Texture* mTexture = nullptr;
    void ConvertMatToTexture();
    void UpdateTexture();
};

#endif // CAMERAFRAME_H

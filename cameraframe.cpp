#include "cameraframe.h"


CameraFrame::CameraFrame(SDL_Renderer* renderer)
{
    mRenderer = renderer;
}

void CameraFrame::SetMatrix(cv::Mat mat)
{
    mMat = mat;
    ConvertMatToTexture();
    UpdateTexture();
}

void CameraFrame::ConvertMatToTexture()
{
    if (mTexture == nullptr)
    {
        mTexture = SDL_CreateTexture(
            mRenderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, mMat.cols,
            mMat.rows);
    }
}

SDL_Texture* CameraFrame::GetTexture()
{
    return mTexture;
}
cv::Mat& CameraFrame::GetMatrix()
{
    return mMat;
}

void CameraFrame::UpdateTexture()
{
    SDL_UpdateTexture(mTexture, NULL, (void*)mMat.data, mMat.step1());
}

int CameraFrame::GetWidth()
{
    return mMat.cols;
}
int CameraFrame::GetHeight()
{
    return mMat.rows;
}

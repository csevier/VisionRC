#include "racer.h"

Racer::Racer() {}

std::string& Racer::GetName()
{
    return mName;
}

void Racer::SetName(std::string name)
{
    mName = name;
}

void Racer::SetColor(float h, float s, float v)
{
    mColorHSV255.w = 255.0f;
    mColorHSV255.x=h;
    mColorHSV255.y=s;
    mColorHSV255.z=v;
    SetUpperColor(h,s,v);
    SetLowerColor(h,s,v);
}

void Racer::SetUpperColor(float h, float s, float v)
{
    mUpperBoundColorHSV255.w = 255.0f;
    mUpperBoundColorHSV255.x=h;
    mUpperBoundColorHSV255.y=s;
    mUpperBoundColorHSV255.z=v;

    mUpperBoundColorHSV1.w = 1.0f;
    mUpperBoundColorHSV1.x=h /255.0;
    mUpperBoundColorHSV1.y=s /255.0;
    mUpperBoundColorHSV1.z=v /255.0;
}

void Racer::SetLowerColor(float h, float s, float v)
{
    mlowerBoundColorHSV255.w = 255.0f;
    mlowerBoundColorHSV255.x=h;
    mlowerBoundColorHSV255.y=s;
    mlowerBoundColorHSV255.z=v;

    mlowerBoundColorHSV1.w = 1.0f;
    mlowerBoundColorHSV1.x=h /255.0;
    mlowerBoundColorHSV1.y=s /255.0;
    mlowerBoundColorHSV1.z=v /255.0;
}

ImVec4& Racer::GetColorHSV255()
{
    return mColorHSV255;
}

ImVec4& Racer::GetUpperColorHSV255()
{
    return mUpperBoundColorHSV255;
}

ImVec4& Racer::GetLowerColorHSV255()
{
    return mlowerBoundColorHSV255;
}

ImVec4& Racer::GetUpperColorHSV1()
{
    return mUpperBoundColorHSV1;
}

ImVec4& Racer::GetLowerColorHSV1()
{
    return mlowerBoundColorHSV1;
}

bool Racer::HasCheckedIn()
{
    return mHasCheckedIn;
}

void Racer::CheckIn()
{
    mHasCheckedIn = true;
}

void Racer::CheckOut()
{
    mHasCheckedIn = false;
}

void Racer::ClockIn(Uint32 clockTime)
{
    mLaptimes.push_back(clockTime - mLastClocked);
    mLastClocked = clockTime;
}

Uint32 Racer::LastClockIn()
{
    return mLastClocked;
}

std::vector<Uint32>& Racer::GetLapTimes()
{
    return mLaptimes;
}

void Racer::Reset()
{
    CheckOut();
    mLastClocked = 0;
    mLaptimes.clear();
}

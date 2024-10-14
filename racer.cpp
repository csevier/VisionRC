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
    if (mLaptimes.size() == 0)
    {
        mLaptimes.push_back(clockTime - mStartedAt);
    }
    else
    {
        mLaptimes.push_back(clockTime - mLastClocked);
    }
    mLastClocked = clockTime;
}

Uint32 Racer::LastClockIn()
{
    return mLastClocked;
}

Uint32 Racer::StartedAt()
{
    return mStartedAt;
}

void Racer::StartedAt(Uint32 startedAt)
{
    mStartedAt = startedAt;
    mLastClocked = startedAt; // skip a lap marker but mark clock in time for delay.
}

std::vector<Uint32>& Racer::GetLapTimes()
{
    return mLaptimes;
}

void Racer::Reset()
{
    CheckOut();
    mLastClocked = 0;
    mStartedAt = 0;
    mLaptimes.clear();
}

bool Racer::IsOverlapping()
{
    return mOverlappingWith.size() != 0;
}

std::vector<Racer>& Racer::GetOverlapping()
{
    return mOverlappingWith;
}

void Racer::SetOverlapping(std::vector<Racer> racers)
{
    mOverlappingWith = racers;
}

void Racer::ClearOverlaps()
{
    mOverlappingWith.clear();
}

bool Racer::HasStarted()
{
    return mStartedAt > 0;
}

char* Racer::GetNotes()
{
    return mNotes;
}

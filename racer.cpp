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
    lapSectionTimes[mLaptimes.size()] = mSectionTimes;
    if (mSectionTimes.size() > 1)// this means zones are in play and more then just a finish line zone.
    {
        Uint32 laptime = 0;
        for (auto time : mSectionTimes)
        {
            laptime += time;
        }
        mLaptimes.push_back(laptime);
    }
    else
    {
        if (mLaptimes.size() == 0)
        {
            mLaptimes.push_back(clockTime - mStartedAt);
        }
        else
        {
            mLaptimes.push_back(clockTime - mLastClocked);
        }
    }
    mSectionTimes.clear();
    mLastClocked = clockTime;
}

void Racer::ClockSection(Uint32 clockTime)
{
    if (mSectionTimes.size() < 1)
    {
        mCurrentZoneTime = clockTime;
        mSectionTimes.push_back(0);
        lastZoneClockTimes[mCurrentZone] = clockTime;
        mLastZoneTime = clockTime;
    }
    else
    {
        mCurrentZoneTime = clockTime;
        Uint32 time = mCurrentZoneTime - mLastZoneTime;
        lastZoneClockTimes[mCurrentZone]= clockTime;
        mSectionTimes.push_back(time);
        mLastZoneTime = mCurrentZoneTime;
    }
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
    lapSectionTimes.clear();
    mSectionTimes.clear();
    mLastZone = -1;
    mLastZoneTime =0;
    mCurrentZone = -1;
    mCurrentZoneTime =0;
    for (int i = 0; i < 25; i++)
    {
        lastZoneClockTimes[i] = 0;
    }
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

int Racer::GetTotalLaps()
{
    return mLaptimes.size();
}

Uint32 Racer::GetTotalTime()
{
    Uint32 totalTime =0;
    for (auto& n : mLaptimes)
    {
        totalTime += n;
    }
    return totalTime;
}

Uint32 Racer::AverageLapTime()
{
    if(GetTotalLaps() == 0)
    {
        return 0;
    }
    return GetTotalTime() / GetTotalLaps();
}

Uint32 Racer::FastestLapTime()
{
    Uint32 fastestLapTime =0;
    if (GetTotalLaps() == 0)
    {
        return fastestLapTime;
    }
    else
    {
        fastestLapTime = mLaptimes[0];
    }

    for (auto& n : mLaptimes)
    {
        if (n <fastestLapTime)
        {
            fastestLapTime = n ;
        }
    }
    return fastestLapTime;
}

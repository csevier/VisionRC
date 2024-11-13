#ifndef RACER_H
#define RACER_H

#include <string>
#include <map>
#include <SDL2/SDL.h>
#include <vector>
#include "imgui.h"

// c api
struct Racer_t
{
    bool mHasCheckedIn = false;
    Uint32 mLastClocked = 0;
    Uint32 mStartedAt = 0;
    std::vector<Uint32> mLaptimes;
    char *mName;
    ImVec4 mlowerBoundColorHSV1 ={0.0f, 0.0f, 0.0f, 1.0f};
    ImVec4 mlowerBoundColorHSV255 = {0.0f, 0.0f, 0.0f, 255.0f};
    ImVec4 mUpperBoundColorHSV1 ={0.0f, 0.0f, 0.0f, 1.0f};
    ImVec4 mUpperBoundColorHSV255 = {0.0f, 0.0f, 0.0f, 255.0f};
    ImVec4 mColorHSV255= {0.0f, 0.0f, 0.0f, 255.0f};
    std::vector<Racer_t> mOverlappingWith;
    char mNotes[1024] = "";
    int mRequiredPixels = 0;
    int mCurrentPixels  = 0;
    bool inFrame = false;
};
// end c api

//c ++ api

class Racer
{
public:
    Racer();
    std::string& GetName();
    void SetName(std::string name);
    void SetColor(float h, float s, float v);
    void SetUpperColor(float h, float s, float v);
    void SetLowerColor(float h, float s, float v);
    ImVec4& GetColorHSV255();
    ImVec4& GetUpperColorHSV255();
    ImVec4& GetLowerColorHSV255();
    ImVec4& GetUpperColorHSV1();
    ImVec4& GetLowerColorHSV1();
    bool HasCheckedIn();
    void CheckIn();
    void CheckOut();
    void ClockIn(Uint32 clockTime);
    void ClockSection(Uint32 clockTime);
    Uint32 LastClockIn();
    void StartedAt(Uint32 startedAt);
    Uint32 StartedAt();
    bool HasStarted();
    std::vector<Uint32>& GetLapTimes();
    void Reset();
    bool IsOverlapping();
    std::vector<Racer>& GetOverlapping();
    void SetOverlapping(std::vector<Racer> racers);
    void ClearOverlaps();
    char* GetNotes();
    int GetTotalLaps();
    Uint32 GetTotalTime();
    Uint32 AverageLapTime();
    Uint32 FastestLapTime();
    Uint32 SlowestLapTime();
    int mRequiredPixels = 0;
    int mCurrentPixels  = 0;
    bool inFrame = false;
    bool racerIsDone = false;
    int mLastZone = -1;
    Uint32 mLastZoneTime =0;
    int mCurrentZone = -1;
    Uint32 mCurrentZoneTime =0;
    std::vector<Uint32> mSectionTimes;
    std::map<int,std::vector<Uint32>> lapSectionTimes;
    Uint32 lastZoneClockTimes[25]= {0};; // 25 zones max.
    void RecalcLapFromSections(int lapIndex);

private:
    bool mHasCheckedIn = false;
    Uint32 mLastClocked = 0;
    Uint32 mStartedAt = 0;
    std::vector<Uint32> mLaptimes;
    std::string mName;
    ImVec4 mlowerBoundColorHSV1 ={0.0f, 0.0f, 0.0f, 1.0f};
    ImVec4 mlowerBoundColorHSV255 = {0.0f, 0.0f, 0.0f, 255.0f};
    ImVec4 mUpperBoundColorHSV1 ={0.0f, 0.0f, 0.0f, 1.0f};
    ImVec4 mUpperBoundColorHSV255 = {0.0f, 0.0f, 0.0f, 255.0f};
    ImVec4 mColorHSV255= {0.0f, 0.0f, 0.0f, 255.0f};
    std::vector<Racer> mOverlappingWith;
    char mNotes[1024] = "";
};
//end cpp api
#endif // RACER_H

#ifndef RACER_H
#define RACER_H

#include <string>
#include <SDL2/SDL.h>
#include <vector>
#include "imgui.h"

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

#endif // RACER_H

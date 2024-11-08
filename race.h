#ifndef RACE_H
#define RACE_H

#include "racer.h"
#include "camera.h"
#include <map>
#include <SDL2/SDL.h>
#include <SDL_mixer.h>
#include <imgui.h>
#include <imfilebrowser.h>

enum RaceStatus
{
    NOT_STARTED,
    CHECKING_IN,
    COUNTING_DOWN,
    RUNNING,
    ENDED
};

class Race
{
public:
    Race();
    void AddRacer(Racer racer);
    void RemoveRacer(std::string racerName);
    std::map<std::string, Racer>& GetRacers();
    void Update(Camera& raceCamera);
    bool Draw();
    RaceStatus GetRaceStatus();
    void StartCheckIn();
    void StartRace();
    void EndRace();
    void Reset();
    void StartCountdown();
    void ExportRace(std::string& location);
    void ExportColors(std::string& location);
    void ImportColors(std::string& location);
    std::vector<Racer> GetRacePositions();
    int lapCount = 1;
    Uint32 countDownStartedAt = 0;
    Uint32 lastCountDownTonePlayedAt = 0;
    bool autoStart = false;

private:
    std::map<std::string, Racer> mRacers;
    std::map<std::string, std::vector<float>> mPositions;
    std::vector<Racer> mTotalRacersInFrame;
    bool mHasOverlappingRacers;
    RaceStatus mStatus;
    Uint32 mCurrentTime = 0;
    Uint32 mRaceEndedAt = 0;
    Uint32 mRaceStartedAt =0;
    Uint32 mRacerClockInDelay = 3000; // milliseconds.
    std::string FormatTime(Uint32 time);
    float GetRacerCurrentPosition(std::string& racerName);
    ImGui::FileBrowser mRecordingDialogue{ImGuiFileBrowserFlags_::ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_::ImGuiFileBrowserFlags_CreateNewDir};
    ImGui::FileBrowser mExportDialogue{ImGuiFileBrowserFlags_::ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_::ImGuiFileBrowserFlags_CreateNewDir};
    ImGui::FileBrowser mRacerColorExportDialogue{ImGuiFileBrowserFlags_::ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_::ImGuiFileBrowserFlags_CreateNewDir};
    ImGui::FileBrowser mRacerColorImportDialogue;
    std::string mSelectRecordingLocation;
    Mix_Chunk* mToneSFX = nullptr;
    Mix_Chunk* mRaceStartedSFX = nullptr;
    Mix_Chunk* mRaceStartingSFX = nullptr;
    Mix_Chunk* mRaceEndedSFX = nullptr;
    Mix_Chunk* mCheckinSFX = nullptr;
    Mix_Chunk* mToneHighSFX = nullptr;
};

extern Race CURRENT_RACE;

#endif // RACE_H

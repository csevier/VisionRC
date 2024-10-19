#ifndef RACE_H
#define RACE_H

#include "racer.h"
#include "camera.h"
#include <map>
#include <SDL2/SDL.h>
#include <imgui.h>
#include <imfilebrowser.h>

enum RaceStatus
{
    NOT_STARTED,
    CHECKING_IN,
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
    void Draw();
    RaceStatus GetRaceStatus();
    void StartCheckIn();
    void StartRace();
    void EndRace();
    void Reset();
    void ExportRace(std::string& location);
    void ExportColors(std::string& location);
    void ImportColors(std::string& location);
    std::vector<Racer> GetRacePositions();

private:
    std::map<std::string, Racer> mRacers;
    RaceStatus mStatus;
    Uint32 mCurrentTime = 0;
    Uint32 mRaceEndedAt = 0;
    Uint32 mRaceStartedAt =0;
    Uint32 mRacerClockInDelay = 3000; // milliseconds.
    std::string FormatTime(Uint32 time);
    ImGui::FileBrowser mRecordingDialogue{ImGuiFileBrowserFlags_::ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_::ImGuiFileBrowserFlags_CreateNewDir};
    ImGui::FileBrowser mExportDialogue{ImGuiFileBrowserFlags_::ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_::ImGuiFileBrowserFlags_CreateNewDir};
    ImGui::FileBrowser mRacerColorExportDialogue{ImGuiFileBrowserFlags_::ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_::ImGuiFileBrowserFlags_CreateNewDir};
    ImGui::FileBrowser mRacerColorImportDialogue;
    std::string mSelectRecordingLocation;
};

extern Race CURRENT_RACE;

#endif // RACE_H

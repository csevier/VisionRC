#include "race.h"
#include <iostream>
#include <chrono>
#include <string>
#include <fstream>
#include <ostream>


bool ComparRacers(Racer racer1, Racer racer2)
{
    if(racer1.GetLapTimes().empty()||racer1.GetLapTimes().empty())
    {
        return racer1.StartedAt() < racer2.StartedAt();
    }

    if(racer1.GetTotalLaps() == racer2.GetTotalLaps())
    {
        return racer1.GetTotalTime() < racer2.GetTotalTime();
    }
    return racer1.GetTotalLaps() > racer2.GetTotalLaps();
}

Race CURRENT_RACE;

Race::Race()
{
    mRecordingDialogue.SetTitle("Offline Recording");
    mRecordingDialogue.SetTypeFilters({ ".mp4"});
    mExportDialogue.SetTitle("Saving Race Results");
    mExportDialogue.SetTypeFilters({ ".txt"});
    mRacerColorExportDialogue.SetTitle("Saving Racer Colors");
    mRacerColorExportDialogue.SetTypeFilters({ ".txt"});
    mRacerColorImportDialogue.SetTitle("Importing Racer Colors");
    mRacerColorImportDialogue.SetTypeFilters({ ".txt"});
    Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2, 4096 );
    mToneSFX = Mix_LoadWAV( "tone.wav" );
    if( mToneSFX == NULL )
    {
        printf( "Failed to load tone effect! SDL_mixer Error: %s\n", Mix_GetError() );
    }
    mToneHighSFX = Mix_LoadWAV( "tone_high.wav" );
    if( mToneHighSFX == NULL )
    {
        printf( "Failed to load tone high effect! SDL_mixer Error: %s\n", Mix_GetError() );
    }

    mRaceEndedSFX = Mix_LoadWAV( "race_ended.wav" );
    if( mRaceEndedSFX == NULL )
    {
        printf( "Failed to load ended effect! SDL_mixer Error: %s\n", Mix_GetError() );
    }

    mCheckinSFX = Mix_LoadWAV( "race_checkin.wav" );
    if( mCheckinSFX == NULL )
    {
        printf( "Failed to load checkin effect! SDL_mixer Error: %s\n", Mix_GetError() );
    }

    mRaceStartingSFX = Mix_LoadWAV( "race_starting.wav" );
    if( mRaceStartingSFX == NULL )
    {
	printf( "Failed to load started effect! SDL_mixer Error: %s\n", Mix_GetError() );
    }
    mStatus = RaceStatus::NOT_STARTED;
}

void Race::AddRacer(Racer racer)
{
    if(racer.GetName().empty() || std::all_of(racer.GetName().begin(),racer.GetName().end(),isspace)) return;
    mRacers[racer.GetName()] = racer;
}

void Race::RemoveRacer(std::string racerName)
{
    mRacers.erase(racerName);
    removedRacers.push_back(racerName);
}

bool Race::Draw(Camera& raceCamera)
{
    bool shouldResetToSource = false;
    ImGui::Begin("Race Configuration");
    ImGui::BeginDisabled(GetRaceStatus() == RaceStatus::RUNNING);
    ImGui::Combo("Race Mode", &mMode, "Laps\0Training\0Time\0\0");
    if (mMode == 0) // laps
    {
        ImGui::SliderInt("Lap Count", &lapCount, 1, 50);
    }
    else if (mMode ==1) // Training
    {
        // race can only end when button is pressed.
    }
    else // time
    {
        ImGui::SliderInt("Time In Minutes", &mRaceTime, 1, 20);
    }
    ImGui::EndDisabled();

    int delayInSeconds  = mRacerClockInDelay / 1000;
    if(ImGui::SliderInt("Racer Delay", &delayInSeconds,0,100))
    {
        mRacerClockInDelay = delayInSeconds *1000;
    }
    if(ImGui::Button("Save Video To"))
    {
        mRecordingDialogue.Open();
    }
    if (!mSelectRecordingLocation.empty())
    {
        ImGui::SameLine();
        if(ImGui::Button("Dont Record"))
        {
            mSelectRecordingLocation.clear();
        }
    }

    if(mRecordingDialogue.HasSelected())
    {
        mSelectRecordingLocation = mRecordingDialogue.GetSelected().string();
        std::filesystem::path file = mSelectRecordingLocation;
        if (!file.has_extension() || file.extension() != ".mp4")
        {
            mSelectRecordingLocation += ".mp4";
        }
        if (!std::filesystem::exists(mSelectRecordingLocation))
        {
            std::ofstream { mSelectRecordingLocation }; // create it.
        }
        mRecordingDialogue.ClearSelected();
    }
    if(!mSelectRecordingLocation.empty())
    {
        ImGui::TextUnformatted(mSelectRecordingLocation.c_str());
    }
    else
    {
        ImGui::TextUnformatted("Not Recording - Select Video To Record.");
    }
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(mSelectRecordingLocation.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    mRecordingDialogue.Display();
    std::string statusText;
    switch (mStatus)
    {
    case RaceStatus::NOT_STARTED:
        ImGui::LabelText("Not Started","Status: ");
        if (ImGui::Button("Start Check-In"))
        {
            StartCheckIn();
        }
        break;
    case RaceStatus::CHECKING_IN:
        ImGui::LabelText("Checking In", "Status: ");
        if (ImGui::Button("Start Race"))
        {
            StartRace();
        }
        break;
    case RaceStatus::COUNTING_DOWN:
        ImGui::LabelText("Counting Down!", "Status: ");
        if (ImGui::Button("New Race"))
        {
            Reset();
        }

	break;
    case RaceStatus::RUNNING:
        ImGui::LabelText("Racing!", "Status: ");
        if (ImGui::Button("End Race"))
        {
            EndRace();
        }
        break;
    case RaceStatus::ENDED:
        ImGui::LabelText("Race ended.", "Status: ");
        if (ImGui::Button("New Race"))
        {
            Reset();
        }
        if (ImGui::Button("Export Results"))
        {
            mExportDialogue.Open();
        }
        break;
    }
    if(mExportDialogue.HasSelected())
    {
         std::string  exportLocation = mExportDialogue.GetSelected().string();
        std::filesystem::path file = exportLocation;
        if (!file.has_extension() || file.extension() != ".txt")
        {
            exportLocation += ".txt";
        }
        ExportRace(exportLocation);
        mExportDialogue.ClearSelected();
    }
    mExportDialogue.Display();
    ImGui::Checkbox("Autostart when all racers check-in.", &autoStart);
    ImGui::End();
    ImGui::Begin("Race Overview");
    if (mMode == 0) // laps
    {
        ImGui::LabelText("Laps", "Mode: ");
        ImGui::LabelText(FormatTime(mCurrentTime).c_str(), "Race Timer: ");
        ImGui::LabelText(std::to_string(lapCount).c_str(), "Laps To Finish: ");
    }
    if (mMode == 1) // training
    {
        ImGui::LabelText("Training", "Mode: ");
        ImGui::LabelText(FormatTime(mCurrentTime).c_str(), "Race Timer: ");
    }
    if (mMode == 2) // timed
    {
        ImGui::LabelText("Time", "Mode: ");
        ImGui::LabelText(FormatTime(mCurrentTime).c_str(), "Race Timer: ");
        ImGui::LabelText(FormatTime((mRaceTime * 60 * 1000) - mCurrentTime).c_str(), "Time Left: ");
    }
    if (ImGui::BeginTable("Race Overview", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {

        ImGui::TableSetupColumn("Position");
        ImGui::TableSetupColumn("Color/Name");
        ImGui::TableSetupColumn("Laps");
        ImGui::TableSetupColumn("Current Lap Time");
        ImGui::TableSetupColumn("Pace");
        ImGui::TableSetupColumn("Average Lap");
        ImGui::TableSetupColumn("Fastest Lap");
        ImGui::TableHeadersRow();

        std::vector<Racer> racerPositions = GetRacePositions();
        for(int i = 0; i < racerPositions.size(); i++)
        {
            Racer racer = racerPositions[i];
            ImGui::TableNextRow();
            for (int column = 0; column < 7; column++)
            {
                ImGui::TableSetColumnIndex(column);
                switch (column)
                {
                case 0:
                    ImGui::TextUnformatted(std::to_string(i+1).c_str());
                    break;
                case 1:
                {
                    float color[3] = {racer.GetColorHSV255().x / 255, racer.GetColorHSV255().y/ 255, racer.GetColorHSV255().z / 255 };
                    ImGui::ColorEdit3(racer.GetName().c_str(),
                                      color,
                                      ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_InputHSV);
                    break;
                }
                case 2:
                    ImGui::TextUnformatted(std::to_string(racer.GetTotalLaps()).c_str());
                    break;
                case 3:
                {
                    ImGui::TextUnformatted(FormatTime(racer.GetCurrentLapTime(mCurrentTime)).c_str());
                    //ImGui::TextUnformatted(racer.Get);
                    break;
                }
                case 4:
                    ImGui::TextUnformatted((std::to_string(racer.GetTotalLaps()) + "/"+ FormatTime(racer.GetTotalTime())).c_str());
                    break;
                case 5:
                    ImGui::TextUnformatted(FormatTime(racer.AverageLapTime()).c_str());
                    break;
                case 6:
                    ImGui::TextUnformatted(FormatTime(racer.FastestLapTime()).c_str());
                    break;
                }
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();

    ImGui::Begin("Detailed Lap Data");
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("Laps", tab_bar_flags))
    {
        for (auto& racer : mRacers)
        {
            if (ImGui::BeginTabItem(racer.second.GetName().c_str()))
            {
                if(racer.second.HasStarted())
                {
                    ImGui::LabelText(FormatTime(racer.second.StartedAt()).c_str(), "Started at: ");
                }
                ImGui::LabelText(FormatTime(racer.second.AverageLapTime()).c_str(), "Lap Average: ");
                if (ImGui::TreeNode("Laps"))
                {
                    for(int lapi = 0; lapi < racer.second.GetLapTimes().size(); lapi++)
                    {
                        // if (lapi == 0)
                        //     ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                        std::string label = racer.second.GetName() + " Lap " + std::to_string(lapi + 1) + ": ";
                        Uint32 lapTime = racer.second.GetLapTimes()[lapi];
                        ImGui::PushID(lapi);
                        bool pushedStyle = false;
                        if (lapTime == racer.second.FastestLapTime())
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0,1,0,1));
                            pushedStyle = true;
                        }
                        else if (lapTime == racer.second.SlowestLapTime())
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(1,0,0,1));
                            pushedStyle = true;
                        }
                        if (ImGui::TreeNode((label + FormatTime(lapTime)).c_str()))
                        {
                            if (racer.second.lapSectionTimes[lapi].size() > 1)
                            {
                                for (int i = 1; i < racer.second.lapSectionTimes[lapi].size(); i++) // starts at 1 because first section start at is always marked 0.
                                {
                                    std::string label = std::to_string(i) + ": ";
                                    Uint32 sectionTime = racer.second.lapSectionTimes[lapi][i];
                                    ImGui::Text(FormatTime(sectionTime).c_str(), label.c_str(),i);
                                    if (raceCamera.mIsOfflineMode && GetRaceStatus() == RaceStatus::ENDED)
                                    {
                                        ImGui::SameLine();
                                        std::string popUpLabel = "Fix Section " + std::to_string(i) + " with: " + FormatTime((raceCamera.markTwo - raceCamera.markOne) * 1000);
                                        if (ImGui::Button(popUpLabel.c_str()))
                                        {
                                            double sectionTime = raceCamera.markTwo - raceCamera.markOne;
                                            racer.second.lapSectionTimes[lapi][i] = sectionTime * 1000;
                                            racer.second.RecalcLapFromSections(lapi);
                                        }
                                    }
                                }
                            }
                            else
                            {
                                if (raceCamera.mIsOfflineMode && GetRaceStatus() == RaceStatus::ENDED)
                                {
                                    ImGui::SameLine();
                                    std::string popUpLabel = "Fix Lap " + std::to_string(lapi)+ " with: " + FormatTime((raceCamera.markTwo - raceCamera.markOne) * 1000);
                                    if (ImGui::Button(popUpLabel.c_str()))
                                    {
                                        double lapTime = raceCamera.markTwo - raceCamera.markOne;
                                        racer.second.GetLapTimes()[lapi] = lapTime * 1000;
                                    }
                                }
                            }
                            ImGui::TreePop();
                        }
                        if (pushedStyle)
                        {
                            ImGui::PopStyleColor(1);
                        }
                       ImGui::PopID();
                    }
                    ImGui::TreePop();
                }
                ImGui::EndTabItem();
            }
        }
        if(ImGui::BeginTabItem("Positions/Pace"))
        {
            std::vector<Racer> racerPositions = GetRacePositions();
            for(int i = 0; i < racerPositions.size(); i++)
            {

                std::string label = std::to_string(i + 1) + ": ";
                ImGui::LabelText(racerPositions[i].GetName().c_str(), "%s", label.c_str());
                ImGui::SameLine();
                std::string totals = std::to_string(racerPositions[i].GetTotalLaps()) + "/"+ FormatTime(racerPositions[i].GetTotalTime());
                ImGui::TextUnformatted(totals.c_str());
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
    if(ImGui::BeginMainMenuBar())
    {
        if(ImGui::BeginMenu("Racers"))
        {
            if(ImGui::MenuItem("Import Racer Colors", NULL, false, mStatus == RaceStatus::NOT_STARTED))
            {
                mRacerColorImportDialogue.Open();
            }
            if(ImGui::MenuItem("Export Racer Colors", NULL, false, mStatus == RaceStatus::NOT_STARTED))
            {
                mRacerColorExportDialogue.Open();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    if(ImGui::BeginMainMenuBar())
    {
        if(ImGui::BeginMenu("Source"))
        {
            if(ImGui::MenuItem("Return To Source Select", NULL, false, mStatus == RaceStatus::NOT_STARTED))
            {
               shouldResetToSource = true; // should tear down and return to source select.
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    // if(ImGui::BeginMainMenuBar())
    // {
    //     if (ImGui::BeginMenu("Window"))
    //     {
    //         if (ImGui::MenuItem("Default Layout"))
    //         {
    //             ImGui::LoadIniSettingsFromDisk("saved.ini");
    //         }
    //         if (ImGui::MenuItem("Save Layout"))
    //         {
    //             ImGui::SaveIniSettingsToDisk("saved.ini");
    //         }
    //         ImGui::EndMenu();
    //     }
    //     ImGui::EndMainMenuBar();
    // }
    ImGui::Begin("Racers");
    if(mRacerColorExportDialogue.HasSelected())
    {
        std::string  exportLocation = mRacerColorExportDialogue.GetSelected().string();
        std::filesystem::path file = exportLocation;
        if (!file.has_extension() || file.extension() != ".txt")
        {
            exportLocation += ".txt";
        }
        ExportColors(exportLocation);
        mRacerColorExportDialogue.ClearSelected();
    }
    if(mRacerColorImportDialogue.HasSelected())
    {
        std::string  importLocation = mRacerColorImportDialogue.GetSelected().string();
        ImportColors(importLocation);
        mRacerColorImportDialogue.ClearSelected();
    }
    mRacerColorExportDialogue.Display();
    mRacerColorImportDialogue.Display();
    static char str0[128] = "";
    std::string racerNameToRemove;
    for (auto& racer : mRacers)
    {
        float color[3] = {racer.second.GetColorHSV255().x / 255, racer.second.GetColorHSV255().y/ 255, racer.second.GetColorHSV255().z / 255 };
        ImGui::ColorEdit3(racer.second.GetName().c_str(),
                          color,
                          ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_InputHSV);

        std::string lower = racer.second.GetName() + " lower bounds";
        if(ImGui::ColorEdit3(lower.c_str(), (float*)& racer.second.GetLowerColorHSV1(),  ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_InputHSV | ImGuiColorEditFlags_NoPicker))
        {
            racer.second.SetLowerColor(racer.second.GetLowerColorHSV1().x *255, racer.second.GetLowerColorHSV1().y *255 ,racer.second.GetLowerColorHSV1().z *255);
        }

        std::string upper = racer.second.GetName() + " upper bounds";
        if(ImGui::ColorEdit3(upper.c_str(), (float*)& racer.second.GetUpperColorHSV1(),  ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_InputHSV | ImGuiColorEditFlags_NoPicker))
        {
            racer.second.SetUpperColor(racer.second.GetUpperColorHSV1().x *255, racer.second.GetUpperColorHSV1().y *255 ,racer.second.GetUpperColorHSV1().z *255);
        }

        std::string notesLabel = racer.second.GetName() + " Notes (1024 characters)";
        ImGui::InputTextMultiline(notesLabel.c_str(), racer.second.GetNotes(), 1024, ImVec2(500,50));
        std::string toleranceLabel = racer.second.GetName() + " Required Pixels";
        ImGui::SliderInt(toleranceLabel.c_str(), &racer.second.mRequiredPixels, 0, 1000);
        if(racer.second.inFrame)
        {
            ImGui::LabelText("True", "Racer In Start/Finish Frame: ");
        }
        else
        {
            ImGui::LabelText("False", "Racer In Start/Finish Frame: ");
        }
        ImGui::LabelText("Zone/Frame Pixel Count", "%s", std::to_string(racer.second.mCurrentPixels).c_str());
        std::string inZonelabel;
        if (racer.second.mCurrentZone != -1)
        {
            if (racer.second.mCurrentZone ==0)
            {
               ImGui::LabelText("Start/Finish", "Racer In Zone: ");
            }
            else {
                ImGui::LabelText(std::to_string(racer.second.mCurrentZone).c_str(), "Racer In Zone: ");
            }
        }
        else
        {
            ImGui::LabelText("Not in Zone", "Racer In Zone: ");
        }

        if (racer.second.mLastZone != -1)
        {
            if (racer.second.mLastZone ==0)
            {
                ImGui::LabelText("Start/Finish", "Last Zone Racer Was In: ");
            }
            else {
                ImGui::LabelText(std::to_string(racer.second.mLastZone).c_str(), "Last Zone Racer Was In: ");
            }
        }
        else
        {
            ImGui::LabelText("Not Seen yet", "Last Zone Racer Was In: ");
        }

        if(GetRaceStatus() == RaceStatus::CHECKING_IN)
        {
            if(racer.second.HasCheckedIn())
            {
                ImGui::LabelText("True", "Has Checked In: ");
            }
            else
            {
                ImGui::LabelText("False", "Has Checked In: ");
            }
            if(mHasOverlappingRacers)
            {
                for (auto& racerInFrame: mTotalRacersInFrame)
                {
                    if (racerInFrame.GetName() == racer.second.GetName())
                    {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(1,0,0,1), "WARNING RACER OVERLAP DETECTED WITH: ");
                        ImGui::SameLine();
                        std::string names;

                        for (auto& overlap: mTotalRacersInFrame)
                        {
                            if (racerInFrame.GetName() == overlap.GetName()) continue;
                            names += overlap.GetName() + " ";
                        }
                        ImGui::TextColored(ImVec4(1,0,0,1), "%s", names.c_str());
                    }
                }
            }
        }

        if (ImGui::Button(("Remove Racer " + racer.second.GetName()).c_str()))
        {
            racerNameToRemove = racer.second.GetName();
        }
    }
    if (!racerNameToRemove.empty())
    {
        RemoveRacer(racerNameToRemove);
    }
    ImGui::NewLine();
    ImGui::InputText("New Racer Name", str0, IM_ARRAYSIZE(str0));
    if(ImGui::Button("Add Racer"))
    {
        Racer newRacer;
        newRacer.SetName(str0);
        memset(str0, 0, sizeof str0);
        AddRacer(newRacer);
    }
    ImGui::End();
    mTotalRacersInFrame.clear();
    return shouldResetToSource;
}

std::map<std::string, Racer>& Race::GetRacers()
{
    return mRacers;
}

RaceStatus Race::GetRaceStatus()
{
    return mStatus;
}

void Race::StartCheckIn()
{
    if (Mix_Playing(4) !=1)
    {
        Mix_PlayChannel(4, mCheckinSFX, 0 );
    }
    mStatus = RaceStatus::CHECKING_IN;
}

void Race::StartRace()
{
    if (Mix_Playing(3) !=1)
    {
        Mix_PlayChannel(3, mToneHighSFX, 0 );
    }
    mRaceStartedAt = SDL_GetTicks();
    mStatus = RaceStatus::RUNNING;
}

void Race::EndRace()
{
    if (Mix_Playing(2) !=1)
    {
        Mix_PlayChannel(2, mRaceEndedSFX, 0 );
    }
    mRaceEndedAt = SDL_GetTicks();
    mStatus = RaceStatus::ENDED;
}

void Race::Reset()
{
    mStatus = RaceStatus::NOT_STARTED;
    mCurrentTime = 0;
    mRaceStartedAt = 0;
    mRaceEndedAt = 0;
    countDownStartedAt =0;
    lastCountDownTonePlayedAt = 0;
    for(auto& racer : mRacers)
    {
        racer.second.Reset();
    }
}

void Race::StartCountdown()
{   
    if (Mix_Playing(2) !=1)
    {
        Mix_PlayChannel(2, mRaceStartingSFX, 0 );
    }

    mStatus = RaceStatus::COUNTING_DOWN;
    countDownStartedAt = SDL_GetTicks();
    lastCountDownTonePlayedAt = SDL_GetTicks();

}

void Race::Update(Camera& raceCamera)
{
    if (!removedRacers.empty())
    {
        for (std::string& racerName: removedRacers)
        {
            raceCamera.RemoveMask(racerName);
        }
        removedRacers.clear();
    }
    if(GetRaceStatus() == RaceStatus::CHECKING_IN)
    {
        bool allRacersCheckedIn = std::all_of(mRacers.begin(), mRacers.end(),[](auto &racer) { return racer.second.HasCheckedIn();});
        if (allRacersCheckedIn && autoStart)
        {
            StartCountdown();
        }
    }
    if(GetRaceStatus() == RaceStatus::COUNTING_DOWN)
    {
        if (SDL_GetTicks() - countDownStartedAt >= 10000) 
        {
            countDownStartedAt = 0;
            lastCountDownTonePlayedAt = 0;
            StartRace();
        }
        else if (SDL_GetTicks() - countDownStartedAt >= 7000 && (SDL_GetTicks() - lastCountDownTonePlayedAt) >1000)
        {
            Mix_PlayChannel(1, mToneSFX, 0 );
            lastCountDownTonePlayedAt = SDL_GetTicks();
        }
    }
    if(GetRaceStatus() == RaceStatus::RUNNING)
    {
        bool allRacersDone = true;
        if (mMode == 0) // laps
        {
            for (auto& racer : mRacers)
            {
                if(racer.second.GetTotalLaps() <  lapCount)
                {
                    allRacersDone = false;
                    racer.second.racerIsDone = false;
                }
                else
                {
                    racer.second.racerIsDone = true;
                }
            }
            if (allRacersDone)
            {
                EndRace();
            }
        }
        if (mMode == 2) // time
        {
            if ( mCurrentTime >= (mRaceTime * 60 * 1000))
            {
                EndRace();
            }
        }

        if (raceCamera.IsVideoOver())
        {
            EndRace();
        }
        raceCamera.Unpause();
        mCurrentTime = SDL_GetTicks() - mRaceStartedAt;
        if(!mSelectRecordingLocation.empty())
        {
            raceCamera.Record(mSelectRecordingLocation);
        }
        else
        {
            raceCamera.StopRecording();
        }
    }
    if(GetRaceStatus() == RaceStatus::ENDED)
    {
        raceCamera.StopRecording();
        raceCamera.Pause();
    }
    for(auto& racer : mRacers)
    {
        if (racer.second.racerIsDone) continue;
        int channel = 1;
        bool racerInFrame = raceCamera.RacerInFrame(racer.second);
        if (racerInFrame) mTotalRacersInFrame.push_back(racer.second);
        if(GetRaceStatus() == RaceStatus::CHECKING_IN && racerInFrame)
        {
            if (!racer.second.HasCheckedIn())
            {
                if (Mix_Playing(channel) !=channel)
                {
                    Mix_PlayChannel(channel, mToneSFX, 0 );
                }
                racer.second.CheckIn();
            }
        }

        if(GetRaceStatus() == RaceStatus::RUNNING
            && racer.second.mCurrentZone != -1
            && (mCurrentTime - racer.second.lastZoneClockTimes[racer.second.mCurrentZone] > mRacerClockInDelay)
            && raceCamera.polyZones.size() > 1
            || (GetRaceStatus() == RaceStatus::RUNNING && racer.second.mSectionTimes.empty()&& racer.second.mCurrentZone ==0)) // call at start.
        {
            racer.second.ClockSection(mCurrentTime);
        }
        if(GetRaceStatus() == RaceStatus::RUNNING && racerInFrame)
        {
            if(racer.second.LastClockIn() == 0)
            {
                if (Mix_Playing(channel) !=channel)
                {
                    Mix_PlayChannel(channel, mToneSFX, 0 );
                }
                racer.second.StartedAt(mCurrentTime);
            }
            else if((mCurrentTime - racer.second.LastClockIn()) > mRacerClockInDelay)
            {
                if (Mix_Playing(channel) !=channel)
                {
                    Mix_PlayChannel(channel, mToneSFX, 0 );
                }
                racer.second.ClockIn(mCurrentTime);
                mPositions[racer.second.GetName()].push_back(GetRacerCurrentPosition(racer.second.GetName()));
            }
        }
        channel += 1;
    }
    mHasOverlappingRacers = mTotalRacersInFrame.size() >1;
}

std::string Race::FormatTime(Uint32 time)
{
    std::chrono::milliseconds ms(time);
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(ms);
    ms -= std::chrono::duration_cast<std::chrono::milliseconds>(secs);
    auto mins = std::chrono::duration_cast<std::chrono::minutes>(secs);
    secs -= std::chrono::duration_cast<std::chrono::seconds>(mins);
    auto hour = std::chrono::duration_cast<std::chrono::hours>(mins);
    mins -= std::chrono::duration_cast<std::chrono::minutes>(hour);
    std::stringstream out;
    if (mins.count() < 10)
    {
        out << "0" << mins.count();
    }
    else
    {
        out << mins.count();
    }
    out << ":";
    if (secs.count() < 10)
    {
        out << "0" << secs.count();
    }
    else
    {
        out << secs.count();
    }
    out << ":";
    if (ms.count() < 10)
    {
        out << "00" << ms.count();
    }
    else if (ms.count() < 100)
    {
        out << "0" << ms.count();
    }
    else
    {
        out << ms.count();
    }
    return out.str();
}

void Race::ExportRace(std::string& location)
{
    std::ofstream out(location);
    out << "Race Duration: " << FormatTime(mCurrentTime) << std::endl << std::endl;
    out << "Race Results: " << std::endl;
    std::vector<Racer> racerPositions = GetRacePositions();
    for(int i = 0; i < racerPositions.size(); i++)
    {
        std::string positionDetails = std::to_string(i + 1) + ": " + racerPositions[i].GetName() + " " + std::to_string(racerPositions[i].GetTotalLaps()) + "/"+ FormatTime(racerPositions[i].GetTotalTime());
        out << positionDetails << std::endl;
    }
    out << std::endl << std::endl;
    for(auto &racer : mRacers)
    {
        out << racer.second.GetName() << std::endl;
        out << "============================" << std::endl;
        out << "Notes:" << std::endl;
        out << "    " << racer.second.GetNotes() << std::endl << std::endl;
        out << "Fastest Lap: " << FormatTime(racer.second.FastestLapTime()) << std::endl;
        out << "Average Lap: " << FormatTime(racer.second.AverageLapTime()) << std::endl << std::endl;
        out << "Laps:" << std::endl;
        out << "    " << racer.second.GetName() + " Started at: " + FormatTime(racer.second.StartedAt()) << std::endl;
        for(int i = 0; i < racer.second.GetLapTimes().size(); i++)
        {
            std::string label = racer.second.GetName() + " Lap " + std::to_string(i + 1) + ": ";
            Uint32 lapTime = racer.second.GetLapTimes()[i];
            out << "    " << label + FormatTime(lapTime) << std::endl;
            if (racer.second.lapSectionTimes[i].size() > 1)
            {
                for (int j =1; j <racer.second.lapSectionTimes[i].size(); j++)
                {
                    std::string label = racer.second.GetName() + " Section  " + std::to_string(j) + ": ";
                    out << "         " << label + FormatTime(racer.second.lapSectionTimes[i][j]) << std::endl;
                }
            }
        }
        out << std::endl;
        out << std::endl;
    }
    out.close();
}

std::vector<Racer> Race::GetRacePositions()
{
    std::vector<Racer> racers;
    for(auto racer: mRacers)
    {
        racers.push_back(racer.second);
    }
    std::sort(racers.begin(), racers.end(),ComparRacers);
    return racers;
}

void Race::ExportColors(std::string& location)
{
    std::ofstream out(location);
    for(auto& racer: mRacers)
    {
        out << racer.second.GetName();
        out << std::endl;
        out << racer.second.GetColorHSV255().x<< " ";
        out << racer.second.GetColorHSV255().y<< " ";
        out << racer.second.GetColorHSV255().z<< " ";
        out << racer.second.GetUpperColorHSV255().x<< " ";
        out << racer.second.GetUpperColorHSV255().y<< " ";
        out << racer.second.GetUpperColorHSV255().z<< " ";
        out << racer.second.GetLowerColorHSV255().x<< " ";
        out << racer.second.GetLowerColorHSV255().y<< " ";
        out << racer.second.GetLowerColorHSV255().z<< " ";
        out << std::to_string(racer.second.mRequiredPixels);
        out << std::endl;
    }
    out.close();
}

void Race::ImportColors(std::string& location)
{
    mRacers.clear();
    std::ifstream in(location);
    std::string instr;
    std::string name;
    std::string baseH;
    std::string baseS;
    std::string baseV;
    std::string upperH;
    std::string upperS;
    std::string upperV;
    std::string lowerH;
    std::string lowerS;
    std::string lowerV;
    std::string requiredPixels;
    char space;
    while(getline(in, name))
    {
        in >> baseH >> baseS >> baseV >> upperH >> upperS >> upperV >> lowerH >> lowerS >> lowerV >> requiredPixels >> space;
        in.seekg(in.tellg() - static_cast<std::istream::pos_type>(1));
        Racer racer;
        racer.SetName(name);
        racer.SetColor(std::stof(baseH),std::stof(baseS),std::stof(baseV));
        racer.SetUpperColor(std::stof(upperH),std::stof(upperS),std::stof(upperV));
        racer.SetLowerColor(std::stof(lowerH),std::stof(lowerS),std::stof(lowerV));
        racer.mRequiredPixels = std::stoi(requiredPixels);
        AddRacer(racer);
    }
    in.close();
}

float Race::GetRacerCurrentPosition(std::string& racerName)
{
    std::vector<Racer> positions = GetRacePositions();
    for(int i=0; i < positions.size(); i++)
    {
        if (positions[i].GetName() == racerName)
        {
            return i + 1;
        }
    }
}



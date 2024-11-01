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
    mRaceEndedSFX = Mix_LoadWAV( "race_ended.wav" );
    if( mRaceEndedSFX == NULL )
    {
        printf( "Failed to load ended effect! SDL_mixer Error: %s\n", Mix_GetError() );
    }
    mRaceStartedSFX = Mix_LoadWAV( "race_started.wav" );
    if( mRaceStartedSFX == NULL )
    {
        printf( "Failed to load started effect! SDL_mixer Error: %s\n", Mix_GetError() );
    }

    mCheckinSFX = Mix_LoadWAV( "race_checkin.wav" );
    if( mCheckinSFX == NULL )
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
}

bool Race::Draw()
{
    bool shouldResetToSource = false;
    ImGui::Begin("Race");
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
        ImGui::Text(mSelectRecordingLocation.c_str());
    }
    else
    {
        ImGui::Text("Not Recording - Select Video To Record.");
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
    ImGui::LabelText(FormatTime(mCurrentTime).c_str(), "Race Timer: ");
    ImGui::End();

    ImGui::Begin("Lap Data");
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("Laps", tab_bar_flags))
    {
        for (auto& racer : mRacers)
        {
            if (ImGui::BeginTabItem(racer.second.GetName().c_str()))
            {
                if (ImGui::BeginTabBar("racer", tab_bar_flags))
                {
                    if (ImGui::BeginTabItem("Laps"))
                    {
                        if(racer.second.HasStarted())
                        {
                            ImGui::LabelText(FormatTime(racer.second.StartedAt()).c_str(), "Started at: ");
                        }
                        for(int i = 0; i < racer.second.GetLapTimes().size(); i++)
                        {
                            std::string label = racer.second.GetName() + " Lap " + std::to_string(i + 1) + ": ";
                            Uint32 lapTime = racer.second.GetLapTimes()[i];
                            if (lapTime == racer.second.FastestLapTime())
                            {
                                std::string fastest = label + FormatTime(lapTime);
                                ImGui::TextColored(ImVec4(0,1,0,1),fastest.c_str());
                            }
                            else
                            {
                                ImGui::LabelText(FormatTime(lapTime).c_str(), label.c_str());
                            }
                        }
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Sections"))
                    {
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
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
                ImGui::LabelText(racerPositions[i].GetName().c_str(), label.c_str());
                ImGui::SameLine();
                std::string totals = std::to_string(racerPositions[i].GetTotalLaps()) + "/"+ FormatTime(racerPositions[i].GetTotalTime());
                ImGui::Text(totals.c_str());
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
        ImGui::SliderInt(toleranceLabel.c_str(), &racer.second.mRequiredPixels, 0, 3000);
        if(racer.second.inFrame)
        {
            ImGui::LabelText("True", "Racer In Frame: ");
        }
        else
        {
            ImGui::LabelText("False", "Racer In Frame: ");
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
                        ImGui::TextColored(ImVec4(1,0,0,1), names.c_str());
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
        Mix_PlayChannel(3, mRaceStartedSFX, 0 );
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
    for(auto& racer : mRacers)
    {
        racer.second.Reset();
    }
}

void Race::Update(Camera& raceCamera)
{
    if(GetRaceStatus() == RaceStatus::RUNNING)
    {
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
        int channel = 1;
        std::vector<int> inZones;
        bool racerInFrame = raceCamera.RacerInFrame(racer.second, inZones);
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
    out << mins.count() << ":" << secs.count() << ":" << ms.count();
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
        out << racer.second.GetName() << " ";
        out << racer.second.GetColorHSV255().x<< " ";
        out << racer.second.GetColorHSV255().y<< " ";
        out << racer.second.GetColorHSV255().z<< " ";
        out << racer.second.GetUpperColorHSV255().x<< " ";
        out << racer.second.GetUpperColorHSV255().y<< " ";
        out << racer.second.GetUpperColorHSV255().z<< " ";
        out << racer.second.GetLowerColorHSV255().x<< " ";
        out << racer.second.GetLowerColorHSV255().y<< " ";
        out << racer.second.GetLowerColorHSV255().z<< " ";
        out << std::to_string(racer.second.mRequiredPixels) << " ";
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
    while(in >> name >> baseH >> baseS >> baseV >> upperH >> upperS >> upperV >> lowerH >> lowerS >> lowerV >> requiredPixels)
    {
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



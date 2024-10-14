#include "race.h"
#include <iostream>
#include <chrono>
#include <string>
#include <fstream>
#include <ostream>

Race CURRENT_RACE;

Race::Race()
{
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

void Race::Draw()
{
    ImGui::Begin("Race");
    int delayInSeconds  = mRacerClockInDelay / 1000;
    if(ImGui::SliderInt("Racer Delay", &delayInSeconds,0,100))
    {
        mRacerClockInDelay = delayInSeconds *1000;
    }
    ImGui::Checkbox("Record Race", &mShouldRecordRace);
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
            ExportRace();
        }
        break;
    }

    ImGui::LabelText(FormatTime(mCurrentTime).c_str(), "Race Timer: ");
    ImGui::End();

    ImGui::Begin("Lap Times");
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
                for(int i = 0; i < racer.second.GetLapTimes().size(); i++)
                {
                    std::string label = racer.second.GetName() + " Lap " + std::to_string(i + 1) + ": ";
                    Uint32 lapTime = racer.second.GetLapTimes()[i];
                    ImGui::LabelText(FormatTime(lapTime).c_str(), label.c_str());
                }
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
    ImGui::End();

    ImGui::Begin("Racers");
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
            if(racer.second.IsOverlapping())
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1,0,0,1), "WARNING RACER OVERLAP DETECTED WITH: ");
                ImGui::SameLine();
                std::string names;



                for(Racer overlap : racer.second.GetOverlapping())
                {
                    names += overlap.GetName() + ", ";
                }
                ImGui::TextColored(ImVec4(1,0,0,1), names.c_str());
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
    mStatus = RaceStatus::CHECKING_IN;
}

void Race::StartRace()
{
    mRaceStartedAt = SDL_GetTicks();
    mStatus = RaceStatus::RUNNING;
}

void Race::EndRace()
{
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
        mCurrentTime = SDL_GetTicks() - mRaceStartedAt;
        if(mShouldRecordRace)
        {
            raceCamera.Record();
        }
        else
        {
            raceCamera.StopRecording();
        }
    }
    if(GetRaceStatus() == RaceStatus::ENDED)
    {
        raceCamera.StopRecording();
    }
    for(auto& racer : mRacers)
    {
        bool racerInFrame = raceCamera.RacerInFrame(racer.second);
        if(GetRaceStatus() == RaceStatus::CHECKING_IN && racerInFrame)
        {
            std::vector<Racer> overlappingRacers;
            for(auto& otherRacers : mRacers) // checking overlap
            {
                bool racerInFrame = raceCamera.RacerInFrame(otherRacers.second);
                if(racerInFrame && otherRacers.second.GetName() != racer.second.GetName())
                {
                    overlappingRacers.push_back(otherRacers.second);
                }
            }
            if(overlappingRacers.size() == 0) // no overlap
            {
                racer.second.CheckIn();
                racer.second.ClearOverlaps();
            }
            else
            {
                racer.second.CheckOut();
                racer.second.SetOverlapping(overlappingRacers);
            } // overlap with
        }
        if(GetRaceStatus() == RaceStatus::RUNNING && racerInFrame)
        {
            if(racer.second.LastClockIn() == 0)
            {
                racer.second.StartedAt(mCurrentTime);
            }
            else if((mCurrentTime - racer.second.LastClockIn()) > mRacerClockInDelay)
            {
                racer.second.ClockIn(mCurrentTime);
            }
        }
    }
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

void Race::ExportRace()
{
    std::ofstream out("race.txt");
    for(auto &racer : mRacers)
    {
        out << racer.second.GetName() + " Started at: " + FormatTime(racer.second.StartedAt()) << std::endl;
        for(int i = 0; i < racer.second.GetLapTimes().size(); i++)
        {
            std::string label = racer.second.GetName() + " Lap " + std::to_string(i + 1) + ": ";
            Uint32 lapTime = racer.second.GetLapTimes()[i];
            out << label + FormatTime(lapTime) << std::endl;
        }
        out << std::endl;
    }
    out.close();
}

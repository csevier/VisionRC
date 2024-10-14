#include "race.h"
#include <iostream>
#include <chrono>
#include <string>

Race CURRENT_RACE;

Race::Race()
{
    mStatus = RaceStatus::NOT_STARTED;
}

void Race::AddRacer(Racer racer)
{
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
                for(int i = 0; i < racer.second.GetLapTimes().size(); i++)
                {
                    std::string label = racer.second.GetName() + " Lap " + std::to_string(i) + ": ";
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
        if(ImGui::ColorEdit3(lower.c_str(), (float*)& racer.second.GetLowerColorHSV1(),  ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_InputHSV))
        {
            racer.second.SetLowerColor(racer.second.GetLowerColorHSV1().x *255, racer.second.GetLowerColorHSV1().y *255 ,racer.second.GetLowerColorHSV1().z *255);
        }

        std::string upper = racer.second.GetName() + " upper bounds";
        if(ImGui::ColorEdit3(upper.c_str(), (float*)& racer.second.GetUpperColorHSV1(),  ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_InputHSV))
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
        mRacers[newRacer.GetName()] = newRacer;
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
    }
    for(auto& racer : mRacers)
    {
        bool racerInFrame = raceCamera.RacerInFrame(racer.second);
        if(GetRaceStatus() == RaceStatus::CHECKING_IN && racerInFrame)
        {
            racer.second.CheckIn();
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

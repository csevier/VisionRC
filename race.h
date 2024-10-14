#ifndef RACE_H
#define RACE_H

#include "racer.h"
#include "camera.h"
#include <map>
#include <SDL2/SDL.h>

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
private:
    std::map<std::string, Racer> mRacers;
    RaceStatus mStatus;
    Uint32 mCurrentTime = 0;
    Uint32 mRaceEndedAt = 0;
    Uint32 mRaceStartedAt =0;
    Uint32 mRacerClockInDelay = 10000; // milliseconds.
    std::string FormatTime(Uint32 time);
    bool mShouldRecordRace;
};

extern Race CURRENT_RACE;

#endif // RACE_H

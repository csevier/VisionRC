#include "app.h"

int main()
{
    App visionIO{};
    visionIO.Initialize_Subsystems();
    visionIO.Run();
}

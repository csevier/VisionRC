#include "app.h"

int main(int argc, char* argv[])
{
    App visionIO{};
    visionIO.Initialize_Subsystems();
    visionIO.Run();
    return 0;
}

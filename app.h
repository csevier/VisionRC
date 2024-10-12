#ifndef APP_H
#define APP_H

#include <SDL2/SDL.h>

class App
{
public:
    App();
    int Initialize_Subsystems();
    int Run();
private:
    SDL_Renderer* mRenderer;
    SDL_Window* mWindow;

};

#endif // APP_H

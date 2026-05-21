#include "Window.h"
#include <iostream>



namespace Lengine {

    Window::Window(std::string windowName, int screenWidth, int screenHeight, unsigned int currentFlags)
    {

        SDL_Init(SDL_INIT_EVERYTHING);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

        // for graphics debugging
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        Create(windowName, screenWidth, screenHeight, currentFlags);
    }

    Window::~Window()
    {
    }

    int Window::Create(std::string windowName, int screenWidth, int screenHeight, unsigned int currentFlags)
    {
        Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE ;

        if (currentFlags & INVISIBLE) {
            flags |= SDL_WINDOW_HIDDEN;
        }

        if (currentFlags & BORDERLESS) {
            flags |= SDL_WINDOW_BORDERLESS;
        }

       
        SDL_DisplayMode mode;
        SDL_GetCurrentDisplayMode(0, &mode);

        if (screenWidth > mode.w) this->screenWidth = mode.w;
        if (screenHeight > mode.h) this->screenHeight = mode.h;

        // MSAA
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);


        _sdlWindow = SDL_CreateWindow(
            windowName.c_str(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            screenWidth,
            screenHeight,
            flags
        );

        if (!_sdlWindow)
            fatalError("SDL Window could not be created");

        _glContext = SDL_GL_CreateContext(_sdlWindow);
        if (!_glContext)
            fatalError("OpenGL context could not be created");

        GLenum error = glewInit();
        if (error != GLEW_OK)
            fatalError("Could not initialise glew");

        SDL_GL_SetSwapInterval(0);

        // FULLSCREEN without any top or bottom bar
        if (currentFlags & FULLSCREEN)
        {
            SDL_DisplayMode mode;
            SDL_GetCurrentDisplayMode(0, &mode);

            SDL_SetWindowDisplayMode(_sdlWindow, &mode);
            SDL_SetWindowFullscreen(_sdlWindow, SDL_WINDOW_FULLSCREEN);
        }


        SDL_GL_GetDrawableSize(_sdlWindow, &screenWidth, &screenHeight);

        

        return 0;
    }


    void Window::swapBuffer() {
      
        SDL_GL_SwapWindow(_sdlWindow);

    }

    void Window::quitWindow() {
        SDL_DestroyWindow(_sdlWindow);
    }



    
}




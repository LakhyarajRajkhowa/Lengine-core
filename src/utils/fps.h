#pragma once
#include <SDL2/SDL.h>
#include <chrono>

#include <string>

#include "logging/LogBuffer.h"


struct FrameStats {
    float fps;
    float msPerFrame;
    float deltaTime;
};

struct RuntimeStats {
    FrameStats frameStats{};
    int targetFPS = 144;
    bool limitFPS = true;
};


inline FrameStats LimitFPS(int targetFPS, bool enable)
{
    using namespace std::chrono;
    static auto lastTime = high_resolution_clock::now();
    static float deltaTime = 0.0f;

    auto now = high_resolution_clock::now();
    deltaTime = duration<float>(now - lastTime).count();

    float targetFrame = (targetFPS > 0) ? 1.0f / targetFPS : 0.0f;

    if (enable && targetFrame > 0.0f && deltaTime < targetFrame) {
        double sleepMs = (targetFrame - deltaTime) * 1000.0;
        SDL_Delay((Uint32)sleepMs);


        // recalc deltaTime after sleep
        now = high_resolution_clock::now();
        deltaTime = duration<float>(now - lastTime).count();
    }

    lastTime = now;

    FrameStats stats;
    stats.msPerFrame = deltaTime * 1000.0f;
    stats.fps = (deltaTime > 0.0f) ? (1.0f / deltaTime) : 0.0f;
    stats.deltaTime = deltaTime;

    DEBUG_LOG_GAP("FPS : " + std::to_string(stats.fps), 1000);

    return stats;
}

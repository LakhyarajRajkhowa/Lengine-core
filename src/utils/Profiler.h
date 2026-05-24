#pragma once

#include <chrono>
#include <iostream>
#include <string>



class ScopedTimer
{
public:
    ScopedTimer(const std::string& name)
        : m_Name(name),
        m_Start(std::chrono::high_resolution_clock::now())
    {}

    ~ScopedTimer()
    {
        auto end = std::chrono::high_resolution_clock::now();

        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(
                end - m_Start)
            .count();

        std::cout
            << "[PROFILE] "
            << m_Name
            << " : "
            << duration
            << " us\n";
    }

private:
    std::string m_Name;

    std::chrono::high_resolution_clock::time_point m_Start;
};
// SSB, v0.01 WIP
// (Workspace)

#pragma once

#include <thread>

#define NUM_WIN_TEST 2

class Workspace
{
public:
    Workspace() {};
    ~Workspace() {};

    // non copyable
    Workspace(const Workspace&) = delete;
    Workspace& operator=(const Workspace&) = delete;

    int             Initialise();
    int             Update();
    int             Render();
    int             Terminate();

private:
    std::thread     m_t[NUM_WIN_TEST];
};

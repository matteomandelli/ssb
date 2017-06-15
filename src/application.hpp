// SSB, v0.01 WIP
// (Application)

#pragma once

class Workspace;

class Application
{
public:
    Application(){}
    ~Application(){}

    // non copyable
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    int Initialise();
    int Run();
    int Terminate();

private:
    Workspace*          m_pWorkspace = nullptr;
};

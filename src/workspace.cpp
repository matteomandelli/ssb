// SSB, v0.01 WIP
// (Workspace)

#include "workspace.hpp"
#include "logger.hpp"
#include "window.hpp"

void WindowThread(int& i)
{
    char name[16];
    snprintf(name, 16, "%d", i + 1);
    Window  window;
    int ret = window.Initialise(name, 800, 600);
    if (ret != 0)
        return;
    
    while (!window.IsClosed())
    {
        window.Update();
        window.Render();
    }
    window.Terminate();
}

int Workspace::Initialise()
{
    
    for (int i = 0; i < NUM_WIN_TEST; ++i)
    {   
        m_t[i] = std::thread(WindowThread, std::ref(i));
    }
    return 0;
}

int Workspace::Terminate()
{
    return 0;
}

int Workspace::Update()
{
    for (int i = 0; i < NUM_WIN_TEST; ++i)
    {
        if (m_t[i].joinable())
            m_t[i].join();
    }
    return -1;
}

int Workspace::Render()
{
    return 0;
}

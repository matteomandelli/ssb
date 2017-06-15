// SSB, v0.01 WIP
// (Application)

#include "application.hpp"
#include "workspace.hpp"
#include "assert.hpp"
#include "logger.hpp"

int Application::Initialise()
{
    int ret = -1;

    m_pWorkspace = new Workspace();
    if (m_pWorkspace != nullptr)
        ret = m_pWorkspace->Initialise();

    return ret;
}

int Application::Run()
{
    int ret = m_pWorkspace->Update();
    if (ret == 0)
    {
        int render = m_pWorkspace->Render();
        ASSERT(render == 0);
    }
    
    return ret;
}

int Application::Terminate()
{
    int ret = -1;

    if (m_pWorkspace != nullptr)
    {
        ret = m_pWorkspace->Terminate();
        delete(m_pWorkspace);
        m_pWorkspace = nullptr;
    }

    return ret;
}

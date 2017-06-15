// SSB, v0.01 WIP
// (Main)

#include "application.hpp"
#include "assert.hpp"
#include "logger.hpp"
#include "config.hpp"

int main(int argc, char* argv[])
{
    int ret = 0;

    ret = Log::Initialise(Log::kLogDump);
    ASSERT(ret == 0);

    // Print to test output color and level
    LOG_DUMP("LOG_DUMP");
    LOG_VERBOSE("LOG_VERBOSE");
    LOG_INFO("LOG_INFO");
    LOG_WARNING("LOG_WARNING");
    LOG_ERROR("LOG_ERROR");

    LOG_INFO("Starting %s", APP_NAME);

    Application app;
    ret = app.Initialise();
    ASSERT(ret == 0);

    while (ret >= 0)
    {
        ret = app.Run();
    }

    ret = app.Terminate();
    ASSERT(ret == 0);;

    LOG_INFO("Closing %s", APP_NAME);

    ret = Log::Terminate();
    ASSERT(ret == 0);

    return ret;
}

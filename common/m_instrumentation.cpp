#include "m_instrumentation.h"

#include "doomfunc.h"

#include "c_dispatch.h"
#include "m_fileio.h"

BEGIN_COMMAND(instenable)
{
    if (argc <= 1)
    {
        PrintFmt(PRINT_HIGH, "Usage: instenable <regex>\n");
        return;
    }
    const size_t enabledCount = TimingInstr::Get().EnableStopwatches(argv[1]);

    PrintFmt(PRINT_HIGH, "Enabled {} stopwatch{}\n", enabledCount, enabledCount == 1 ? "" : "es");
}
END_COMMAND(instenable)

BEGIN_COMMAND(instdisable)
{
    if (argc <= 1)
    {
        PrintFmt(PRINT_HIGH, "Usage: instdisable <regex>\n");
        return;
    }
    const size_t disabledCount = TimingInstr::Get().DisableStopwatches(argv[1]);

    PrintFmt(PRINT_HIGH, "Disabled {} stopwatch{}\n", disabledCount, disabledCount == 1 ? "" : "es");
}
END_COMMAND(instdisable)

BEGIN_COMMAND(instrecord)
{
    if (argc <= 1)
    {
        PrintFmt(PRINT_HIGH, "Usage: instrecord <filename>\n");
        return;
    }

    std::string path = M_GetWriteDir();
    if (!M_IsPathSep(path.back()))
    {
        path += PATHSEP;
    }

    const std::string filepath = path + argv[1];

    TimingInstr::Get().StartRecording(path + argv[1]);

    PrintFmt(PRINT_HIGH, "Recording stopwatches to {}\n", filepath);
}
END_COMMAND(instrecord)

BEGIN_COMMAND(inststop)
{
    TimingInstr::Get().StopRecording();
}
END_COMMAND(inststop)

std::shared_ptr<Stopwatch> TimingInstr::CreateStopwatch(const std::string& i_name)
{
    return m_manager.CreateStopwatch(i_name);
}

size_t TimingInstr::EnableStopwatches(const std::string& i_regex)
{
    auto watches = m_manager.Find(i_regex);
    for (auto& watch : watches)
    {
        watch.second->Enable();
    }

    return watches.size();
}

size_t TimingInstr::DisableStopwatches(const std::string& i_regex)
{
    auto watches = m_manager.Find(i_regex);
    for (auto& watch : watches)
    {
        watch.second->Disable();
    }

    return watches.size();
}

bool TimingInstr::StartRecording(const std::string& i_filename)
{
    return m_manager.StartRecording(i_filename);
}

void TimingInstr::ManageRecording(int i_tic)
{
    m_manager.RecordSamples(i_tic);
}

void TimingInstr::StopRecording()
{
    m_manager.StopRecording();
}


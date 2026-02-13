#include "m_instrumentation.h"

#include "doomfunc.h"

#include "c_dispatch.h"

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


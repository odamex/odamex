#pragma once

#include "Stopwatch.h"
#include "StopwatchManager.h"

class TimingInstr
{
    public:
        static TimingInstr& Get()
        {
            static TimingInstr s_instance;
            return s_instance;
        }

        size_t EnableStopwatches(const std::string& i_regex);
        size_t DisableStopwatches(const std::string& i_regex);

        bool StartRecording(const std::string& i_filename);
        void ManageRecording(int i_tic);
        void StopRecording();

    protected:
        TimingInstr() = default;

        StopwatchManager m_manager;
};

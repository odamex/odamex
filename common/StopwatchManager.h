#pragma once

#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

//class Recorder;
class Stopwatch;

class StopwatchManager
{
    public:
        typedef std::map<std::string, std::shared_ptr<Stopwatch> > WatchMap;

        std::shared_ptr<Stopwatch> CreateStopwatch(const std::string& i_name);

        std::shared_ptr<Stopwatch> Get(const std::string& i_name);

        std::vector<WatchMap::value_type> Find(const std::string& i_expression);

        void EnableAll();
        void DisableAll();

        bool StartRecording(const std::string& i_filepath);
        void RecordSamples(int i_tic);
        void StopRecording();

    protected:

        WatchMap                                m_watches;
        std::unique_ptr<std::ofstream>          m_outFile;
        std::vector<std::shared_ptr<Stopwatch>> m_activeRecordingWatches;
        //std::unique_ptr<Recorder> m_recorder;
};


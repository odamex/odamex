#include "StopwatchManager.h"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <regex>

#include "Stopwatch.h"

//        std::map<std::string, std::shared_ptr<Stopwatch> > m_watches;


std::shared_ptr<Stopwatch> StopwatchManager::CreateStopwatch(const std::string& i_name)
{
    auto& existingWatch = m_watches[i_name];

    if (not existingWatch)
    {
        // We get better cache friendliness and tiniest performance bump by using
        // make_shared, which can allocate the control block and Stopwatch in the
        // same memory object, rather than as separate regions.
        existingWatch = std::make_shared<Stopwatch>(i_name);
    }

    return existingWatch;
}

std::shared_ptr<Stopwatch> StopwatchManager::Get(const std::string& i_name)
{
    auto iter = m_watches.find(i_name);

    if (iter != m_watches.end())
    {
        return iter->second;
    }

    return std::make_shared<Stopwatch>(i_name);
}

std::vector<StopwatchManager::WatchMap::value_type> StopwatchManager::Find(const std::string& i_expression)
{
    std::vector<WatchMap::value_type> result;

    try
    {
        std::regex pattern(i_expression,
                           std::regex_constants::icase |
                           std::regex_constants::nosubs |
                           std::regex_constants::optimize );    // Slower construction is worth faster matches here.

        std::smatch matchResult;
        for (auto& value : m_watches)
        {
            if (std::regex_match(value.first,
                                 matchResult,
                                 pattern))
            {
                result.emplace_back(value);
            }
        }
    }
    catch(std::regex_error& e)
    {
        std::cout << i_expression << ": invalid: " << e.what() << std::endl;
    }
    return result;
}

void StopwatchManager::EnableAll()
{
    for (auto& value : m_watches)
    {
        value.second->Enable();
    }
}

void StopwatchManager::DisableAll()
{
    for (auto& value : m_watches)
    {
        value.second->Disable();
    }
}

bool StopwatchManager::StartRecording(const std::string& i_filepath)
{
    StopRecording();

    for (auto& watchMapValue : m_watches)
    {
        if (watchMapValue.second->IsEnabled())
        {
            m_activeRecordingWatches.push_back(watchMapValue.second);
        }
    }

    m_outFile = std::make_unique<std::ofstream>(i_filepath);

    if (m_outFile->good())
    {
        const std::string headingLine = std::accumulate(m_activeRecordingWatches.begin(),
                                                        m_activeRecordingWatches.end(),
                                                        std::string("gametic"),
                                                        [](const std::string& str, auto& watchPtr)
                                                        {
                                                            return str + ',' + watchPtr->GetName();
                                                        });

        *m_outFile << headingLine << std::endl;
        return true;
    }

    m_outFile.reset();
    return false;
}

void StopwatchManager::RecordSamples(int i_gametic)
{
    if (m_outFile)
    {
        const std::string dataLine = std::accumulate(m_activeRecordingWatches.begin(),
                                                     m_activeRecordingWatches.end(),
                                                     std::to_string(i_gametic),
                                                     [](const std::string& str, auto& watchPtr)
                                                     {
                                                         return str + ',' + std::to_string(watchPtr->Duration());
                                                     });
        *m_outFile << dataLine << std::endl;
    }
}

void StopwatchManager::StopRecording()
{
    m_outFile.reset();
    m_activeRecordingWatches.clear();
}

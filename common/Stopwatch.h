#pragma once

#include <cstdint>
#include <iso646.h>

#include "i_time.h"

class Stopwatch
{
    public:

        explicit Stopwatch(const std::string& i_name) :
            m_name      (i_name),
            m_startTime (0),
            m_endTime   (0),
            m_isRunning (false),
            m_isEnabled (false)
        {
        }

        const std::string& GetName() const
        {
            return m_name;
        }

        // Enables time measurement using this stopwatch.
        void Enable()
        {
            m_isEnabled = true;
        }

        void Disable()
        {
            m_isEnabled = false;
        }

        bool IsEnabled() const
        {
            return m_isEnabled;
        }

        /// Begin a new time interval.
        /// Has no effect if the stopwatch is already running.
        void Start()
        {
            if (m_isEnabled and not m_isRunning)
            {
                m_startTime = I_GetTime();
                m_endTime   = 0;
                m_isRunning = true;
            }
        }

        /// Resume measuring the current time interval.
        /// Has no effect if the stopwatch is already running.
        void Resume()
        {
            if (m_isEnabled)
            {
                m_endTime   = 0;
                m_isRunning = true;
            }
        }

        /// End the current time interval.
        void Stop()
        {
            if (m_isEnabled and m_isRunning)
            {
                m_endTime   = I_GetTime();
                m_isRunning = false;
            }
        }

        void Reset()
        {
            if (m_isEnabled)
            {
                m_startTime = m_isRunning ? I_GetTime() : 0;
                m_endTime = 0;
            }
        }

        /// Returns the current time interval length.
        ///
        /// If the stopwatch is running, 0 is returned.
        /// If the stopwatch hasn't yet been started, 0 is returned.
        uint64_t Duration() const
        {
            return (m_isRunning ? 0 : m_endTime - m_startTime);
        }

    protected:
        std::string m_name;
        uint64_t    m_startTime = 0;
        uint64_t    m_endTime   = 0;
        bool        m_isRunning = false;
        bool        m_isEnabled = false;
};

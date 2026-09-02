// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Jim Thoenen.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//  Simple moving average utility.
//
//-----------------------------------------------------------------------------
#pragma once

#include <optional>

template <typename SampleType, size_t POWER_OF_TWO_SAMPLES>
class MovingAverage
{
	public:

		std::optional<SampleType> Update(const SampleType& i_value)
		{
			m_sum += i_value;

			if (m_remainingRequiredSamples == 0)
			{
				m_sum -= m_average;
				m_average = m_sum >> POWER_OF_TWO_SAMPLES;
				return m_average;
			}

			if (--m_remainingRequiredSamples == 0)
			{
				m_average = m_sum >> POWER_OF_TWO_SAMPLES;
				return m_average;
			}
			return std::nullopt;
		}

	protected:
		size_t       m_remainingRequiredSamples { 1 << POWER_OF_TWO_SAMPLES };
		SampleType   m_average                  { 0 };
		SampleType   m_sum                      { 0 };
};

#pragma once

#include "p_pspr.h"

struct PspriteStateType
{
	statenum_t  statenum;
	int         tics;
	//fixed_t     sx      { 0};
	//fixed_t     sy      { 0};

    PspriteStateType() :
        statenum{-1},
        tics    {-1}
    {
    }

    PspriteStateType(statenum_t i_statenum, int i_tics) :
        statenum{i_statenum},
        tics    {i_tics}
    {
    }

    bool operator==(const PspriteStateType&) const = default;

    PspriteStateType& operator=(const pspdef_t& pspdef)
    {
        statenum = pspdef.state ? pspdef.state->statenum : static_cast<statenum_t>(-1);
        tics     = pspdef.tics;
        return *this;
    }

	template <typename StreamType>
	friend StreamType& operator<<(StreamType& io_stream, const PspriteStateType& i_thisRef)
	{
		io_stream
		    << i_thisRef.statenum
		    << i_thisRef.tics
		    ;
		return io_stream;
	}

	template <typename StreamType>
	friend StreamType& operator>>(StreamType& io_stream, PspriteStateType& o_thisRef)
	{
		io_stream
		    >> o_thisRef.statenum
		    >> o_thisRef.tics
		    ;
		return io_stream;
	}

    PspriteStateType Next() const
    {
        // non-existent or frozen psprites have themselves as the next in the sequence.
        if (statenum == -1 or tics == -1)
        {
            return *this;
        }

        // Now proceed for valid states.  tics == 0 is invalid because we never
        // "stop" at a 0-tics state - we only slide through them until we get a state that
        // has a non-zero tic, even -1.
        auto iter = ::states.find(statenum);
        if (iter != ::states.end() and tics != 0)
        {
            int nextTic = tics - 1;
            while (nextTic == 0)
            {
                iter = ::states.find(iter->second.nextstate);
                if (iter == ::states.end())
                {
                    // bail out and return default if the nextstate is invalid.  Should never happen, but
                    // a bad dehacked patch can do it.
                    return PspriteStateType{};
                }
                nextTic = iter->second.tics;
            }

            return PspriteStateType {iter->second.statenum, nextTic};
        }
        return PspriteStateType{};
    }
};

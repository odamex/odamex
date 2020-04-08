#ifndef __G_EVENT_H__
#define __G_EVENT_H__

#include <cstring>
#include <vector>
#include "hashtable.h"
#include "m_ostring.h"


class Event
{
public:
    Event(const OString& event_type) :
        mEventType(event_type), mArgs(8)
    {}

    struct EventVariant
    {
        EventVariant() : mType(TYPE_NOT_FOUND), mAsInteger(0)
        {}

        ~EventVariant()
        {}

        EventVariant(const EventVariant& other)
        {
            operator=(other);
        }

        EventVariant& operator=(const EventVariant& other)
        {
            memcpy((uint8_t*)this, (uint8_t*)&other, sizeof(other));
            return *this;
        }

        enum EventVariantType
        {
            TYPE_NOT_FOUND,
            TYPE_INTEGER,
            TYPE_FLOAT,
            TYPE_BOOL,
            TYPE_STRING,
            TYPE_COUNT // number of unique types
        };

        EventVariantType mType;

        union {
            int32_t     mAsInteger;
            float       mAsFloat;
            bool        mAsBool;
            OString     mAsString;
        };
    };

    bool hasParam(const OString& arg) const
    {
        return mArgs.find(arg) != mArgs.end();
    }

    EventVariant getParam(const OString& arg) const
    {
        EventArgsTable::const_iterator it = mArgs.find(arg);
        if (it != mArgs.end())
            return it->second;
        else
            return EventVariant();
    }

    void setParam(const OString& arg, int32_t value)
    {
        EventVariant ev;
        ev.mType = EventVariant::TYPE_INTEGER;
        ev.mAsInteger = value;
        mArgs.insert(std::make_pair(arg, ev));
    }

    void setParam(const OString& arg, float value)
    {
        EventVariant ev;
        ev.mType = EventVariant::TYPE_FLOAT;
        ev.mAsFloat = value;
        mArgs.insert(std::make_pair(arg, ev));
    }

    void setParam(const OString& arg, bool value)
    {
        EventVariant ev;
        ev.mType = EventVariant::TYPE_BOOL;
        ev.mAsBool = value;
        mArgs.insert(std::make_pair(arg, ev));
    }

    void setParam(const OString& arg, const OString& value)
    {
        EventVariant ev;
        ev.mType = EventVariant::TYPE_STRING;
        ev.mAsString = value;
        mArgs.insert(std::make_pair(arg, ev));
    }

    const OString& getEventType() const
    {
        return mEventType;
    }

private:
    OString         mEventType;

    typedef OHashTable<OString, Event::EventVariant> EventArgsTable;
    EventArgsTable  mArgs;
};


class EventManager
{
public:
    typedef void (*EventHandlerFunction)(Event&);

    static void registerHandler(EventHandlerFunction func)
    {
        getInstance()->mEventHandlers.push_back(func);
    }

    static void dispatchEvent(Event& event)
    {
        const EventHandlerFunctionList& handlers = getInstance()->mEventHandlers;
        for (EventHandlerFunctionList::const_iterator it = handlers.begin(); it != handlers.end(); ++it)
        {
            EventHandlerFunction func = *it;
            func(event);
        }
    }

private:
    EventManager()
    {}

    static EventManager* getInstance() 
    {
        static EventManager instance;
        return &instance;
    }

    typedef std::vector<EventHandlerFunction> EventHandlerFunctionList;
    EventHandlerFunctionList mEventHandlers;
};


#endif // __G_EVENT_H__
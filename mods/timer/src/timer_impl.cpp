#include <stdexcept>
#include <functional>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include "Timer.hpp"


class Timer : public modlib::Timer
{
public:
    TimerID setTimer (
        Tick     delay,
        Callback callback,
        Stage    stage,
        Type     type
    ) override
    {
        if (callback == nullptr)
        {
            throw std::runtime_error ("invalid callback provided");
        }

        if (delay == 0)
        {
            throw std::runtime_error ("timer delay must be greater than zero");
        }

        Tick tickStamp = m_tickCounter + delay;
        TimerID id = ++m_lastID;

        m_callbacksEntries[id] = {
            .m_stage    = stage,
            .m_base     = tickStamp,
            .m_cycle    = (type == Type::CYCLE) ? delay : 0,
            .m_callback = callback
        };
    
        m_tickStamps[tickStamp].emplace (id);
        return id;
    }

    void cancelTimer (
        TimerID& id
    ) override
    {
        if (id == 0)
        {
            return;
        }

        auto entryIterator =  m_callbacksEntries.find (id);
        if ( entryIterator == m_callbacksEntries.end ())
        {
            return;
        }

        CallbackEntry& entry = entryIterator->second;

        Tick tickStamp = entry.m_cycle == 0 ? 
            entry.m_base :
            m_tickCounter < entry.m_base ?
                entry.m_base :
                entry.m_base + ((m_tickCounter - entry.m_base) / entry.m_cycle + 1) * entry.m_cycle + 1;

        m_tickStamps[tickStamp].erase (id);
        m_callbacksEntries.erase (id);
        id = 0;
    }

    size_t tick () override
    {
        m_tickCounter++;

        auto bucketIterator = m_tickStamps.find (m_tickCounter);
        if (bucketIterator == m_tickStamps.end ())
        {
            return 0;
        }
        auto& bucket = bucketIterator->second;

        size_t emitted = bucket.size ();
        std::queue<Callback> callbackQueue;

        for (TimerID id : bucket)
        {
            const auto& entry = m_callbacksEntries[id]; // An entry with this ID always exists in m_callbacksEntries

            if (entry.m_cycle > 0)
            {
                Tick nextTick = m_tickCounter + entry.m_cycle;
                m_tickStamps[nextTick].emplace (id);
            }

            if (entry.m_stage == Stage::ON_UPDATE)
            {
                try
                {
                    entry.m_callback ();
                }
                catch (const std::exception& e)
                {
                    emitted--;

                    std::cerr << "[T" << m_tickCounter << "][Timer][TID" << id << "] Callback threw an exception: " << e.what () << std::endl;
                }
            }
            else
            {
                callbackQueue.push (entry.m_callback);
            }   
        }
        m_tickStamps.erase (bucketIterator);

        while (!callbackQueue.empty ())
        {
            try
            {
                callbackQueue.front () ();
            }
            catch (const std::exception& e)
            {
                emitted--;
                std::cerr << "[Timer] Callback threw an exception: " << e.what () << std::endl;
            }
            callbackQueue.pop ();
        }

        return emitted;
    }

    Tick             getTicksSinceCreation ()       override { return m_tickCounter; }

    std::string_view id                    () const override { return "DoDoeb.timer_mngr"; }
    std::string_view brief                 () const override { return "Singleton timer module by DoDoeb"; }
    ModVersion       version               () const override { return ModVersion (1, 2, 0); }

private:
    struct CallbackEntry
    {
        Stage    m_stage;
        Tick     m_base;
        Tick     m_cycle;
        Callback m_callback;
    };


    Tick m_tickCounter = 0;
    TimerID m_lastID = 0;

    std::unordered_map<TimerID, CallbackEntry>            m_callbacksEntries;
    std::unordered_map<Tick, std::unordered_set<TimerID>> m_tickStamps;
};


extern "C" Mod *modlib_create (ModManager *)
{
    return new Timer();
}

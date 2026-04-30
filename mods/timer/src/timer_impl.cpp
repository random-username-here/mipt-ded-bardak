#include <stdexcept>
#include <functional>
#include <map>
#include <set>
#include <cstring>
#include <queue>
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

        Tick tickStamp = m_tickCounter + delay;

        TimerID id = {
            .m_type = type,
            .m_tick = delay,
            .m_ID   = ++m_lastID
        };

        m_tickStamps[tickStamp][id.m_ID] = {
            .m_stage    = stage,
            .m_cycle    = type == Type::CYCLE ? delay : 0,
            .m_callback = callback
        };
        
        
        return id;
    }

    void cancelTimer (
        TimerID& id
    ) override
    {
        m_tickStamps[id.m_tick].erase (id.m_ID);

        std::memset (&id, 0, sizeof (TimerID));
    }

    void tick () override
    {
        m_tickCounter++;

        if (m_tickStamps[m_tickCounter].empty () == false)
        {
            std::queue<Callback> callbackbackQueue;

            for (const auto [id, entry] : m_tickStamps[m_tickCounter])
            {
                if (entry.m_stage == Stage::ON_UPDATE)
                {
                    entry.m_callback ();
                }
                else
                {
                    callbackbackQueue.push (entry.m_callback);
                }
                if (entry.m_cycle)
                {
                    m_tickStamps[m_tickCounter + entry.m_cycle][id] = entry;
                }
            }
            m_tickStamps.erase (m_tickCounter);

            while (callbackbackQueue.empty () == false)
            {
                callbackbackQueue.front () ();
                callbackbackQueue.pop ();
            }
        }
    }

    Tick             getTicksSinceCreation ()       override { return m_tickCounter; }

    std::string_view id                    () const override { return "DoDoeb.timer_mngr"; }
    std::string_view brief                 () const override { return "Singleton timer module by DoDoeb"; }
    ModVersion       version               () const override { return ModVersion (1, 2, 0); }

private:
    struct CallbackEntry
    {
        Stage    m_stage;
        Tick     m_cycle;
        Callback m_callback;
    };

    
    Tick m_tickCounter = 0;
    std::map<Tick, std::map<uint64_t, CallbackEntry>> m_tickStamps;
};


extern "C" Mod *modlib_create (ModManager *mm)
{
    return new Timer();
}

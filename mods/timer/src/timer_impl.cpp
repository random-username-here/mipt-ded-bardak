#include <stdexcept>
#include <functional>
#include <map>
#include <queue>
#include <unordered_set>
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
        if (id.m_ID == 0)
        {
            return;
        }

        m_cancelledIDs.insert (id.m_ID);

        // remove scheduled timers
        for (auto it = m_tickStamps.begin (); it != m_tickStamps.end ();)
        {
            it->second.erase (id.m_ID);

            if (it->second.empty ())
            {
                it = m_tickStamps.erase (it);
            }
            else
            {
                ++it;
            }
        }

        id = {
            .m_type = Type::COUNTDOWN,
            .m_tick = 0,
            .m_ID   = 0
        };
    }

    void tick () override
    {
        m_tickCounter++;

        auto tickCurrent = m_tickStamps.find (m_tickCounter);
        if (tickCurrent == m_tickStamps.end ())
        {
            return;
        }

        auto callbacks = std::move (tickCurrent->second);
        m_tickStamps.erase (tickCurrent);

        std::queue<std::pair<uint64_t, CallbackEntry>> callbackbackQueue;

        for (const auto &[id, entry] : callbacks)
        {
            if (m_cancelledIDs.count (id))
            {
                continue;
            }

            if (entry.m_stage == Stage::ON_UPDATE)
            {
                entry.m_callback ();

                // cyclic timer could cancel itself

                if (entry.m_cycle && !m_cancelledIDs.count(id))
                {
                    m_tickStamps[m_tickCounter + entry.m_cycle][id] = entry;
                }
            }
            else
            {
                callbackbackQueue.push ({id, entry});
            }
        }

        while (callbackbackQueue.empty () == false)
        {
            uint64_t         id = callbackbackQueue.front().first;
            CallbackEntry entry = callbackbackQueue.front().second;

            callbackbackQueue.pop ();

            if (m_cancelledIDs.count (id))
            {
                continue;
            }

            entry.m_callback ();

            if (entry.m_cycle && !m_cancelledIDs.count (id))
            {
                m_tickStamps[m_tickCounter + entry.m_cycle][id] = entry;
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
    uint64_t m_lastID  = 0;

    std::map<Tick, std::map<uint64_t, CallbackEntry>> m_tickStamps;
    std::unordered_set<uint64_t> m_cancelledIDs;
};


extern "C" Mod *modlib_create (ModManager *mm)
{
    return new Timer();
}

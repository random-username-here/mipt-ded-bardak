#include <stdexcept>
#include <functional>
#include <queue>
#include <unordered_map>
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

        Tick tickStamp = m_tickCounter + id.m_tick - (m_tickCounter % id.m_tick);

        m_tickStamps[tickStamp].erase (id.m_ID);

        id = {
            .m_type = Type::COUNTDOWN,
            .m_tick = 0,
            .m_ID   = 0,
        };
    }

    size_t tick () override
    {
        m_tickCounter++;

        auto& callbacksEntries = m_tickStamps[m_tickCounter];

        size_t emitted = callbacksEntries.size ();
        std::queue<Callback> callbackQueue;

        for (auto& [id, entry] : callbacksEntries)
        {
            if (entry.m_stage == Stage::ON_UPDATE)
            {
                try
                {
                    entry.m_callback ();
                }
                catch (const std::exception& e)
                {
                    emitted--;

                    auto time = std::chrono::system_clock::now ();
                    std::cerr << "[Timer][TID" << id << "] Callback threw an exception: " << e.what () << std::endl;
                }
            }
            else
            {
                callbackQueue.push (entry.m_callback);
            }

            if (entry.m_cycle > 0)
            {
                Tick nextTick = m_tickCounter + entry.m_cycle;
                m_tickStamps[nextTick][id] = entry;
            }
        }

        while (!callbackQueue.empty ())
        {
            try
            {
                callbackQueue.front () ();
            }
            catch (const std::exception& e)
            {
                emitted--;
                auto time = std::chrono::system_clock::now ();
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
        Tick     m_cycle;
        Callback m_callback;
    };


    Tick m_tickCounter = 0;
    uint64_t m_lastID  = 0;

    std::unordered_map<Tick, std::unordered_map<uint64_t, CallbackEntry>> m_tickStamps;
};


extern "C" Mod *modlib_create (ModManager *)
{
    return new Timer();
}

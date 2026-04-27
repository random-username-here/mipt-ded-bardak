#include <stdexcept>
#include <random>
#include <queue>
#include "../timer_impl.hpp"


std::string_view 
Timer::id () const
{
    return "DoDoeb.timer_mngr";
}

std::string_view 
Timer::brief () const
{
    return "Singleton timer module by DoDoeb";
}

ModVersion
Timer::version () const
{
    return ModVersion (1, 1, 6);
}


Timer::TimerID 
Timer::setTimer (
    Tick delay,
    Callback callback,
    Stage type
)
{
    if (callback == nullptr)
    {
        throw std::runtime_error ("invalid callback provided");
    }

    Tick tickStamp = m_tickCounter + delay;

    TimerID id = {
        tickStamp,
        rand ()
    };

    m_tickStamps[tickStamp][id.second] = {
        type,
        callback
    };
    
    
    return id;
}

void
Timer::cancelTimer (
    Timer::TimerID& id
)
{
    m_tickStamps[id.first].erase (id.second);

    id = {0, 0};
}


void
Timer::tick ()
{
    m_tickCounter++;

    if (m_tickStamps[m_tickCounter].empty () == false)
    {
        std::queue<Callback> callbackbackQueue;

        for (const auto [_, entry] : m_tickStamps[m_tickCounter])
        {
            if (entry.first == Stage::ON_UPDATE)
            {
                entry.second ();
            }
            else
            {
                callbackbackQueue.push (entry.second);
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

Timer::Tick
Timer::getTicksSinceCreation ()
{
    return m_tickCounter;
}



extern "C" Mod *modlib_create (ModManager *mm)
{
    return new Timer();
}

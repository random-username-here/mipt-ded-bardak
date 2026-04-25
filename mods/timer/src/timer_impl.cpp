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
    return "I wanna sleep, dont awake me";
}

ModVersion
Timer::version () const
{
    return ModVersion (1, 0, 0);
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

    Tick stamp = ticksSinceCreation + delay;

    CallbackEntry entry = {
        .type = type,
        .callback = callback
    };

    stamps[stamp].push_back (entry);
    
    return TimerID (stamp, std::prev (stamps[stamp].end()));
}

void
Timer::cancelTimer (
    Timer::TimerID id
)
{
    stamps[id.stamp].erase (id.entryIterator);
}


void
Timer::tick ()
{
    ticksSinceCreation++;

    if (stamps[ticksSinceCreation].empty () == false)
    {
        std::queue<Callback> callbackbackQueue;

        for (const CallbackEntry& entry : stamps[ticksSinceCreation])
        {
            if (entry.type == Stage::IMMEDIATELY)
            {
                entry.callback ();
            }
            else
            {
                callbackbackQueue.push (entry.callback);
            }
        }
        stamps.erase (ticksSinceCreation);

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
    return ticksSinceCreation;
}

extern "C" Mod *
modlib_create (ModManager *mm)
{
    return new Timer();
}

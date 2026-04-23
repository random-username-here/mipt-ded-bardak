#include <stdexcept>
#include <random>
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
    Callback callback
)
{
    if (callback == nullptr)
    {
        throw std::runtime_error ("invalid callback provided");
    }

    TimerID timerID = rand ();
    Tick tickStamp = getTicksSinceCreation () + delay;

    stampBus[tickStamp][timerID] = callback;
    
    return timerID;
}

void
Timer::cancelTimer (
    Timer::TimerID id
)
{
    for (auto [timestamp, timers] : stampBus)
    {
        auto timer = timers.find (id);
        if (timer != timers.end ())
        {
            timers.erase (timer);
            if (timers.empty ())
            {
                stampBus.erase (timestamp);
            }
        }
    }
}


void
Timer::tick ()
{
    ticksSinceCreation++;

    if (stampBus[ticksSinceCreation].empty () == false)
    {
        for (auto [_, callback] : stampBus[ticksSinceCreation])
        {
            callback ();
        }
        stampBus.erase (ticksSinceCreation);
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

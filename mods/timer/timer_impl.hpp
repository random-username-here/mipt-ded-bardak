#pragma once


#include <functional>
#include <map>
#include <unordered_map>
#include "Timer.hpp"


class Timer : public modlib::Timer
{
public:
    using TimerID   = uint64_t;
    using Callback  = std::function<void(void)>;
    using Tick      = size_t;

    TimerID 
    setTimer (
        Tick delay,
        Callback callback
    );

    void
    cancelTimer (
        TimerID id
    );

    void
    tick ();

    Tick
    getTicksSinceCreation ();

    std::string_view id      () const override;
    std::string_view brief   () const override;
    ModVersion       version () const override;
    

private:
    Tick ticksSinceCreation = 0;

    std::unordered_map<Tick, std::map<TimerID, Callback>> stampBus;
};

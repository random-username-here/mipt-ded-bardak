#pragma once


#include <functional>
#include <list>
#include <unordered_map>
#include "Timer.hpp"


class Timer : public modlib::Timer
{
    using Tick = size_t;
    using Callback = std::function<void(void)>;
    enum class Stage
    {
        IMMEDIATELY = 0,
        ONSERVERUPDATE
    };

private:
    struct CallbackEntry
    {
        Stage type;
        std::function<void(void)> callback;
    };

    Tick ticksSinceCreation = 0;

    std::unordered_map<Tick, std::list<CallbackEntry>> stamps;

public:
    class TimerID
    {
        friend class Timer;

    private:
        Tick stamp;
        std::list<CallbackEntry>::iterator entryIterator;

        TimerID (Tick stamp, std::list<CallbackEntry>::iterator entryIterator);
    };

    TimerID setTimer (
        Tick delay,
        Callback callback,
        Stage type
    );

    void cancelTimer (
        TimerID id
    );

    void tick ();

    Tick getTicksSinceCreation ();

    std::string_view id      () const override;
    std::string_view brief   () const override;
    ModVersion       version () const override;
};

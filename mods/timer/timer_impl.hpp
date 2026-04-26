#pragma once


#include <functional>
#include <map>
#include "Timer.hpp"


class Timer : public modlib::Timer
{
public:
    TimerID setTimer (
        Tick delay,
        Callback callback,
        Stage type
    ) override;

    void cancelTimer (
        TimerID& id
    ) override;

    void tick () override;

    Tick getTicksSinceCreation () override;

    std::string_view id      () const override;
    std::string_view brief   () const override;
    ModVersion       version () const override;

private:
    using CallbackEntry = std::pair<Stage, Callback>;

    Tick m_tickCounter = 0;
    std::map<Tick, std::map<uint64_t, CallbackEntry>> m_tickStamps;
};

#pragma once
#include "modlib_mod.hpp"
#include <functional>
#include <utility>

namespace modlib {

class Timer : public Mod
{
public:
    enum class Stage
    {
        ON_UPDATE = 0,
        ON_UPDATE_DONE
    };
    enum class Type
    {
        COUNTDOWN = 0,
        CYCLE
    };
    using Tick     = size_t;
    using Callback = std::function<void(void)>;
    struct TimerID
    {
        Type m_type;
        Tick m_tick;
        uint64_t m_ID;
    };

    virtual TimerID setTimer (
        Tick     delay,
        Callback callback,
        Stage    stage = Stage::ON_UPDATE,
        Type     type   = Type::COUNTDOWN
    ) = 0;

    virtual void cancelTimer (
        TimerID& id
    ) = 0;

    virtual void tick () = 0;

    virtual Tick getTicksSinceCreation () = 0;
};

};

#include "modlib_mod.hpp"
#include <functional>
#include <utility>

namespace modlib {

class Timer : public Mod
{
public:
    using Callback = std::function<void(void)>;
    enum class Stage
    {
        ON_UPDATE = 0,
        ON_UPDATE_DONE
    };
    using Tick = size_t;
    using TimerID = std::pair<Tick, uint64_t>;

    virtual TimerID setTimer (
        Tick delay,
        Callback callback,
        Stage type
    ) = 0;

    virtual void cancelTimer (
        TimerID& id
    ) = 0;

    virtual void tick () = 0;

    virtual Tick getTicksSinceCreation () = 0;
};

};

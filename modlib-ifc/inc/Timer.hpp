#include "modlib_mod.hpp"
#include <functional>

namespace modlib {

class Timer : public Mod
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

public:
    class TimerID
    {
        friend class Timer;

    private:

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
};

};

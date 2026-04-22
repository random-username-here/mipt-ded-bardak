#include "modlib_mod.hpp"
#include <functional>

namespace modlib {

class Timer : public Mod {
public:
    using TimerID = uint64_t;
    using Callback = std::function<void(void)>;

    virtual TimerID setTimer(size_t delay, Callback callback) = 0;
    virtual void cancelTimer(TimerID id) = 0;
    virtual void tick() = 0;

    virtual size_t getTicksSinceCreation() = 0;
};

};

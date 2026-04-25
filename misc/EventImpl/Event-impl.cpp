#include "Event.hpp"

using namespace Event;


template<typename ...Args>
Entry<Args...>::SID Entry<Args...>::subscribe (Callback callback)
{
    callbackBus.push_back (callback);

    return SID (std::prev (callbackBus.end()));
}

template<typename ...Args>
void Entry<Args...>::unsubscribe (SID subscriptionID)
{
    callbackBus.erase (subscriptionID.it);
}

template<typename ...Args>
uint16_t Entry<Args...>::emit (Args... args)
{
    uint16_t emitted = 0;
    
    for (const auto& callback : callbackBus)
    {
        callback (args...);
        emitted++;
    }

    return emitted;
}

template<typename ...Args>
const std::list<typename Entry<Args...>::Callback>& Entry<Args...>::getSubscriptions () const
{
    return callbackBus;
}


#pragma once


#include <list>
#include <functional>


template<typename ...Args>
class Event
{
public:
    using Callback = std::function<void(Args...)>;
    class SID
    {
        friend class Entry;

    public:
        SID (std::list<Callback>::iterator subscriptionIterator) : it(subscriptionIterator) {}

    private:
        std::list<Callback>::iterator it;
    };

    SID subscribe (Callback callback)
    {
        callbackBus.push_back (callback);

        return SID (std::prev (callbackBus.end()));
    }

    void unsubscribe (SID subscriptionID)
    {
        callbackBus.erase (subscriptionID.it);
    }

    size_t emit (Args... args)
    {   
        for (const auto& callback : callbackBus)
        {
            callback (args...);
        }

        return callbackBus.size ();
    }

    const std::list<Callback>& getSubscriptions () const
    {
        return callbackBus;
    }
    
private:
    std::list<Callback> callbackBus;
};
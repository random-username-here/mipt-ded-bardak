#pragma once

#include "binmsg.hpp"
#include <list>
#include <functional>


namespace Event
{
    template<typename ...Args>
    class Entry
    {
        using Callback = std::function<void(Args...)>;
        class SID
        {
            friend class Entry;

        public:
            SID (std::list<Callback>::iterator subscriptionIterator) : it(subscriptionIterator) {}

        private:
            std::list<Callback>::iterator it;
        };

    public:
        SID    subscribe (Callback callback);
        void unsubscribe (SID      subscriptionID);

        uint16_t emit (Args... args);

        const std::list<Callback>& getSubscriptions () const;
        
    private:
        std::list<Callback> callbackBus;
    };

    
}
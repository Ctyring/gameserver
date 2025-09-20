#pragma once

#include "handler_manager.h"
#include <array>
#include <cstdint>

namespace cfl{
    struct EventParam
    {
        uint32_t eventId{};
        std::array<uint32_t, 2> intParams{};
        std::array<uint64_t, 2> longParams{};
    };

    class EventHandlerManager : public HandlerManager
    {
    private:
        EventHandlerManager() = default;
        virtual ~EventHandlerManager() = default;

    public:
        EventHandlerManager(const EventHandlerManager&) = delete;
        EventHandlerManager& operator=(const EventHandlerManager&) = delete;

        static EventHandlerManager& instance()
        {
            static EventHandlerManager instance;
            return instance;
        }

        template<typename TClass>
        bool registerEventHandler(
                int eventId,
                bool (TClass::*func)(EventParam*),
                TClass* object)
        {
            return registerMessageHandler(eventId, func, object);
        }

        template<typename TClass>
        bool unregisterEventHandler(int eventId, TClass* object)
        {
            return unregisterMessageHandler(eventId, object);
        }

        bool fireEvent(
                uint32_t eventId,
                uint32_t param1 = 0,
                uint32_t param2 = 0,
                uint64_t longParam1 = 0,
                uint64_t longParam2 = 0)
        {
            EventParam param{
                    .eventId = eventId,
                    .intParams = {param1, param2},
                    .longParams = {longParam1, longParam2}
            };

            return fireMessage(eventId, &param);
        }
    };
}

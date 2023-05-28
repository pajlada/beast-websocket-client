#pragma once

#include "messages/metadata.hpp"
#include "payloads/channel-ban-v1.hpp"
#include "payloads/session-welcome.hpp"

namespace eventsub {

class Listener
{
public:
    virtual ~Listener() = default;

    virtual void onSessionWelcome(
        messages::Metadata metadata,
        payload::session_welcome::Payload payload) = 0;

    virtual void onChannelBan(messages::Metadata metadata,
                              payload::channel_ban::v1::Payload payload) = 0;
};

}  // namespace eventsub

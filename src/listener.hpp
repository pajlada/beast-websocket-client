#pragma once

#include "messages/metadata.hpp"
#include "payloads/channel-ban-v1.hpp"
#include "payloads/session-welcome.hpp"
#include "payloads/stream-offline-v1.hpp"
#include "payloads/stream-online-v1.hpp"

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
    virtual void onStreamOnline(
        messages::Metadata metadata,
        payload::stream_online::v1::Payload payload) = 0;
    virtual void onStreamOffline(
        messages::Metadata metadata,
        payload::stream_offline::v1::Payload payload) = 0;
};

}  // namespace eventsub

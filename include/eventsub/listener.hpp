#pragma once

#include "eventsub/messages/metadata.hpp"
#include "eventsub/payloads/channel-ban-v1.hpp"
#include "eventsub/payloads/channel-chat-notification-beta.hpp"
#include "eventsub/payloads/channel-update-v1.hpp"
#include "eventsub/payloads/session-welcome.hpp"
#include "eventsub/payloads/stream-offline-v1.hpp"
#include "eventsub/payloads/stream-online-v1.hpp"

namespace eventsub {

class Listener
{
public:
    virtual ~Listener() = default;

    virtual void onSessionWelcome(
        messages::Metadata metadata,
        payload::session_welcome::Payload payload) = 0;

    // Subscription types
    virtual void onChannelBan(messages::Metadata metadata,
                              payload::channel_ban::v1::Payload payload) = 0;

    virtual void onStreamOnline(
        messages::Metadata metadata,
        payload::stream_online::v1::Payload payload) = 0;

    virtual void onStreamOffline(
        messages::Metadata metadata,
        payload::stream_offline::v1::Payload payload) = 0;

    virtual void onChannelChatNotification(
        messages::Metadata metadata,
        payload::channel_chat_notification::beta::Payload payload) = 0;

    virtual void onChannelUpdate(
        messages::Metadata metadata,
        payload::channel_update::v1::Payload payload) = 0;

    // Add your new subscription types above this line
};

}  // namespace eventsub

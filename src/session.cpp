#include "eventsub/session.hpp"

#include "eventsub/listener.hpp"
#include "eventsub/messages/metadata.hpp"
#include "eventsub/payloads/channel-ban-v1.hpp"
#include "eventsub/payloads/session-welcome.hpp"

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/container_hash/hash.hpp>
#include <boost/json.hpp>

#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <unordered_map>

namespace beast = boost::beast;  // from <boost/beast.hpp>

namespace eventsub {

// Subscription Type + Subscription Version
using EventSubSubscription = std::pair<std::string, std::string>;

using NotificationHandlers = std::unordered_map<
    EventSubSubscription,
    std::function<void(messages::Metadata, boost::json::value,
                       std::unique_ptr<Listener> &)>,
    boost::hash<EventSubSubscription>>;

using MessageHandlers = std::unordered_map<
    std::string, std::function<void(messages::Metadata, boost::json::value,
                                    std::unique_ptr<Listener> &,
                                    const NotificationHandlers &)>>;

namespace {

// Report a failure
void fail(beast::error_code ec, char const *what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

template <class T>
std::optional<T> parsePayload(const boost::json::value &jv)
{
    auto result = try_value_to<T>(jv);
    if (!result.has_value())
    {
        fail(result.error(), "parsing payload");
        return std::nullopt;
    }

    return result.value();
}

// Subscription types
const NotificationHandlers NOTIFICATION_HANDLERS{
    {
        {"channel.ban", "1"},
        [](const auto &metadata, const auto &jv, auto &listener) {
            auto oPayload =
                parsePayload<eventsub::payload::channel_ban::v1::Payload>(jv);
            if (!oPayload)
            {
                return;
            }
            listener->onChannelBan(metadata, *oPayload);
        },
    },
    {
        {"stream.online", "1"},
        [](const auto &metadata, const auto &jv, auto &listener) {
            auto oPayload =
                parsePayload<eventsub::payload::stream_online::v1::Payload>(jv);
            if (!oPayload)
            {
                return;
            }
            listener->onStreamOnline(metadata, *oPayload);
        },
    },
    {
        {"stream.offline", "1"},
        [](const auto &metadata, const auto &jv, auto &listener) {
            auto oPayload =
                parsePayload<eventsub::payload::stream_offline::v1::Payload>(
                    jv);
            if (!oPayload)
            {
                return;
            }
            listener->onStreamOffline(metadata, *oPayload);
        },
    },
    {
        {"channel.chat.notification", "beta"},
        [](const auto &metadata, const auto &jv, auto &listener) {
            auto oPayload = parsePayload<
                eventsub::payload::channel_chat_notification::beta::Payload>(
                jv);
            if (!oPayload)
            {
                return;
            }
            listener->onChannelChatNotification(metadata, *oPayload);
        },
    },
    {
        {"channel.update", "1"},
        [](const auto &metadata, const auto &jv, auto &listener) {
            auto oPayload =
                parsePayload<eventsub::payload::channel_update::v1::Payload>(
                    jv);
            if (!oPayload)
            {
                return;
            }
            listener->onChannelUpdate(metadata, *oPayload);
        },
    },
    // Add your new subscription types above this line
};

const MessageHandlers MESSAGE_HANDLERS{
    {
        "session_welcome",
        [](const auto &metadata, const auto &jv, auto &listener,
           const auto & /*notificationHandlers*/) {
            auto oPayload = parsePayload<payload::session_welcome::Payload>(jv);
            if (!oPayload)
            {
                // TODO: error handling
                return;
            }
            const auto &payload = *oPayload;

            listener->onSessionWelcome(metadata, payload);
        },
    },
    {
        "session_keepalive",
        [](const auto &metadata, const auto &jv, auto &listener,
           const auto &notificationHandlers) {
            // TODO: should we do something here?
        },
    },
    {
        "notification",
        [](const auto &metadata, const auto &jv, auto &listener,
           const auto &notificationHandlers) {
            listener->onNotification(metadata, jv);

            if (!metadata.subscriptionType || !metadata.subscriptionVersion)
            {
                // TODO: error handling
                return;
            }

            auto it = notificationHandlers.find(
                {*metadata.subscriptionType, *metadata.subscriptionVersion});
            if (it == notificationHandlers.end())
            {
                // TODO: error handling
                return;
            }

            it->second(metadata, jv, listener);
        },
    },
};

void handleMessage(std::unique_ptr<Listener> &listener,
                   const beast::flat_buffer &buffer,
                   boost::json::error_code &ec)
{
    auto jv = boost::json::parse(beast::buffers_to_string(buffer.data()), ec);
    if (ec)
    {
        // TODO: wrap error?
        return;
    }

    const auto *jvObject = jv.if_object();
    if (jvObject == nullptr)
    {
        // TODO: set error
        return;
    }

    const auto *metadataV = jvObject->if_contains("metadata");
    if (metadataV == nullptr)
    {
        // TODO: set error
        return;
    }
    auto metadataResult = try_value_to<messages::Metadata>(*metadataV);
    if (metadataResult.has_error())
    {
        // TODO: wrap error
        ec = metadataResult.error();
        return;
    }

    const auto &metadata = metadataResult.value();

    auto handler = MESSAGE_HANDLERS.find(metadata.messageType);

    if (handler == MESSAGE_HANDLERS.end())
    {
        // TODO: set error
        return;
    }

    const auto *payloadV = jvObject->if_contains("payload");
    if (payloadV == nullptr)
    {
        // TODO: set error
        return;
    }

    handler->second(metadata, *payloadV, listener, NOTIFICATION_HANDLERS);
}

}  // namespace

boost::asio::awaitable<void> sessionReader(WebSocketStream &ws,
                                           std::unique_ptr<Listener> listener)
{
    for (;;)
    {
        // This buffer will hold the incoming message
        beast::flat_buffer buffer;

        // Read a message into our buffer
        boost::system::error_code readError;
        auto result = co_await ws.async_read(
            buffer,
            boost::asio::redirect_error(boost::asio::use_awaitable, readError));
        if (readError)
        {
            fail(readError, "read");
            break;
        }

        boost::json::error_code ec;
        handleMessage(listener, buffer, ec);
        if (ec)
        {
            // TODO: error handling xd
        }
    }
}

}  // namespace eventsub

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
#include <format>
#include <iostream>
#include <memory>
#include <unordered_map>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;

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
        {"channel.chat.notification", "1"},
        [](const auto &metadata, const auto &jv, auto &listener) {
            auto oPayload = parsePayload<
                eventsub::payload::channel_chat_notification::v1::Payload>(jv);
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

}  // namespace

boost::json::error_code handleMessage(std::unique_ptr<Listener> &listener,
                                      const beast::flat_buffer &buffer)
{
    boost::json::error_code parseError;
    auto jv =
        boost::json::parse(beast::buffers_to_string(buffer.data()), parseError);
    if (parseError)
    {
        // TODO: wrap error?
        return parseError;
    }

    const auto *jvObject = jv.if_object();
    if (jvObject == nullptr)
    {
        static const error::ApplicationErrorCategory errorRootMustBeObject{
            "Payload root must be an object"};
        return boost::system::error_code{129, errorRootMustBeObject};
    }

    const auto *metadataV = jvObject->if_contains("metadata");
    if (metadataV == nullptr)
    {
        static const error::ApplicationErrorCategory
            errorRootMustContainMetadata{
                "Payload root must contain a metadata field"};
        return boost::system::error_code{129, errorRootMustContainMetadata};
    }
    auto metadataResult = try_value_to<messages::Metadata>(*metadataV);
    if (metadataResult.has_error())
    {
        // TODO: wrap error?
        return metadataResult.error();
    }

    const auto &metadata = metadataResult.value();

    auto handler = MESSAGE_HANDLERS.find(metadata.messageType);

    if (handler == MESSAGE_HANDLERS.end())
    {
        error::ApplicationErrorCategory errorNoMessageHandlerForMessageType{
            std::format("No message handler found for message type: {}",
                        metadata.messageType)};
        return boost::system::error_code{129,
                                         errorNoMessageHandlerForMessageType};
    }

    const auto *payloadV = jvObject->if_contains("payload");
    if (payloadV == nullptr)
    {
        static const error::ApplicationErrorCategory
            errorRootMustContainPayload{
                "Payload root must contain a payload field"};
        return boost::system::error_code{129, errorRootMustContainPayload};
    }

    handler->second(metadata, *payloadV, listener, NOTIFICATION_HANDLERS);

    return {};
}

// Resolver and socket require an io_context
Session::Session(boost::asio::io_context &ioc, boost::asio::ssl::context &ctx,
                 std::unique_ptr<Listener> listener)
    : resolver_(boost::asio::make_strand(ioc))
    , ws_(boost::asio::make_strand(ioc), ctx)
    , listener(std::move(listener))
{
}

// Start the asynchronous operation
void Session::run(std::string host, std::string port, std::string path,
                  std::string _userAgent)
{
    // Save these for later
    this->host_ = std::move(host);
    this->port_ = std::move(port);
    this->path_ = std::move(path);
    this->userAgent = std::move(_userAgent);

    // Look up the domain name
    this->resolver_.async_resolve(
        this->host_, this->port_,
        beast::bind_front_handler(&Session::onResolve, shared_from_this()));
}

void Session::onResolve(beast::error_code ec,
                        boost::asio::ip::tcp::resolver::results_type results)
{
    if (ec)
    {
        return fail(ec, "resolve");
    }

    // Set a timeout on the operation
    beast::get_lowest_layer(this->ws_).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(this->ws_).async_connect(
        results,
        beast::bind_front_handler(&Session::onConnect, shared_from_this()));
}

void Session::onConnect(
    beast::error_code ec,
    boost::asio::ip::tcp::resolver::results_type::endpoint_type ep)
{
    if (ec)
    {
        return fail(ec, "connect");
    }

    // Set a timeout on the operation
    beast::get_lowest_layer(this->ws_).expires_after(std::chrono::seconds(30));

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(this->ws_.next_layer().native_handle(),
                                  this->host_.c_str()))
    {
        ec = beast::error_code(static_cast<int>(::ERR_get_error()),
                               boost::asio::error::get_ssl_category());
        return fail(ec, "connect");
    }

    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    // See https://tools.ietf.org/html/rfc7230#section-5.4
    host_ += ':' + std::to_string(ep.port());

    // Perform the SSL handshake
    this->ws_.next_layer().async_handshake(
        boost::asio::ssl::stream_base::client,
        beast::bind_front_handler(&Session::onSSLHandshake,
                                  shared_from_this()));
}

void Session::onSSLHandshake(beast::error_code ec)
{
    if (ec)
    {
        return fail(ec, "ssl_handshake");
    }

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(this->ws_).expires_never();

    // Set suggested timeout settings for the websocket
    this->ws_.set_option(
        websocket::stream_base::timeout::suggested(beast::role_type::client));

    // Set a decorator to change the User-Agent of the handshake
    this->ws_.set_option(websocket::stream_base::decorator(
        [userAgent{this->userAgent}](websocket::request_type &req) {
            req.set(http::field::user_agent, userAgent);
        }));

    // Perform the websocket handshake
    this->ws_.async_handshake(
        this->host_, this->path_,
        beast::bind_front_handler(&Session::onHandshake, shared_from_this()));
}

void Session::onHandshake(beast::error_code ec)
{
    if (ec)
    {
        return fail(ec, "handshake");
    }

    this->ws_.async_read(buffer_, beast::bind_front_handler(
                                      &Session::onRead, shared_from_this()));
}

void Session::onRead(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec)
    {
        return fail(ec, "read");
    }

    auto messageError = handleMessage(this->listener, this->buffer_);
    if (messageError)
    {
        return fail(messageError, "handleMessage");
    }

    this->buffer_.clear();

    this->ws_.async_read(buffer_, beast::bind_front_handler(
                                      &Session::onRead, shared_from_this()));
}

/**
    this->ws_.async_close(
        websocket::close_code::normal,
        beast::bind_front_handler(&Session::onClose, shared_from_this()));
        */
void Session::onClose(beast::error_code ec)
{
    if (ec)
    {
        return fail(ec, "close");
    }

    // If we get here then the connection is closed gracefully

    // The make_printable() function helps print a ConstBufferSequence
    std::cout << beast::make_printable(buffer_.data()) << std::endl;
}

}  // namespace eventsub

#include "messages/metadata.hpp"
#include "payloads/channel-ban-v1.hpp"
#include "payloads/session-welcome.hpp"

#include <boost/asio.hpp>
#include <boost/asio/as_tuple.hpp>
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

namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace http = beast::http;            // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace net = boost::asio;             // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;        // from <boost/asio/ssl.hpp>

using boost::asio::awaitable;
using boost::asio::buffer;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::ip::tcp;
using namespace boost::asio::experimental::awaitable_operators;
using std::chrono::steady_clock;
using namespace std::literals::chrono_literals;

using WebSocketStream = websocket::stream<beast::ssl_stream<beast::tcp_stream>>;

constexpr auto use_nothrow_awaitable =
    boost::asio::as_tuple(boost::asio::use_awaitable);

using boost::json::try_value_to_tag;
using boost::json::value;
using boost::json::value_to_tag;

using namespace eventsub;

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

// Subscription Type + Subscription Version
using EventSubSubscription = std::pair<std::string, std::string>;

awaitable<void> sessionReader(WebSocketStream &ws)
{
    std::unordered_map<
        EventSubSubscription,
        std::function<void(twitch::eventsub::Metadata, boost::json::value)>,
        boost::hash<EventSubSubscription>>
        notificationHandlers{
            {
                {"channel.ban", "1"},
                [](const auto &metadata, const auto &jv) {
                    std::cout << "channel.ban!!!\n";
                    auto oPayload = parsePayload<
                        eventsub::payload::channel_ban::v1::Payload>(jv);
                    if (!oPayload)
                    {
                        std::cerr << "bad payload for channel.ban v1\n";
                        return;
                    }
                    const auto &payload = *oPayload;

                    std::cout << "channel.ban reason:" << payload.event.reason
                              << "\n";
                },
            },
        };

    std::unordered_map<
        std::string,
        std::function<void(twitch::eventsub::Metadata, boost::json::value)>>
        handlers{
            {
                "session_welcome",
                [](const auto &metadata, const auto &jv) {
                    std::cout << "SESSION WELCOME\n";

                    auto oPayload =
                        parsePayload<payload::session_welcome::Payload>(jv);
                    if (!oPayload)
                    {
                        std::cerr << "bad payload for session welcome\n";
                        return;
                    }
                    const auto &payload = *oPayload;

                    std::cout << "sessionWelcome id:" << payload.id << "\n";
                },
            },
            {
                "session_keepalive",
                [](const auto &metadata, const auto &jv) {
                    std::cout << "Session keepalive\n";
                },
            },
            {
                "notification",
                [&notificationHandlers](const auto &metadata, const auto &jv) {
                    if (!metadata.subscriptionType ||
                        !metadata.subscriptionVersion)
                    {
                        std::cerr << "missing subscriptionType or "
                                     "subscriptionVersion\n";
                        return;
                    }

                    std::cout << "Received notification for subscription "
                              << *metadata.subscriptionType << ", version "
                              << *metadata.subscriptionVersion << "\n";

                    auto it = notificationHandlers.find(
                        {*metadata.subscriptionType,
                         *metadata.subscriptionVersion});
                    if (it == notificationHandlers.end())
                    {
                        std::cerr << "No notification handler for "
                                  << *metadata.subscriptionType << "\n";
                        return;
                    }

                    it->second(metadata, jv);
                },
            },
        };

    for (;;)
    {
        // This buffer will hold the incoming message
        beast::flat_buffer buffer;

        // Read a message into our buffer
        auto [readError, _bytes_read] =
            co_await ws.async_read(buffer, use_nothrow_awaitable);
        if (readError)
        {
            fail(readError, "read");
            continue;
        }

        boost::json::error_code ec;
        auto jv =
            boost::json::parse(beast::buffers_to_string(buffer.data()), ec);
        if (ec)
        {
            fail(ec, "parsing");
            continue;
        }

        const auto *jvObject = jv.if_object();
        if (jvObject == nullptr)
        {
            std::cerr << "root value was not a json object\n";
            continue;
        }

        const auto *metadataV = jvObject->if_contains("metadata");
        if (metadataV == nullptr)
        {
            std::cerr << "root value was missing the `metadata` key\n";
            continue;
        }
        auto metadataResult =
            try_value_to<twitch::eventsub::Metadata>(*metadataV);
        if (metadataResult.has_error())
        {
            fail(metadataResult.error(), "parsing metadata");
            continue;
        }

        const auto &metadata = metadataResult.value();

        auto handler = handlers.find(metadata.messageType);

        if (handler == handlers.end())
        {
            std::cout << "No handler found for " << metadata.messageType
                      << "\n";

            std::cout << "read: " << beast::make_printable(buffer.data())
                      << std::endl;
            continue;
        }

        std::cout << "read: " << beast::make_printable(buffer.data())
                  << std::endl;

        const auto *payloadV = jvObject->if_contains("payload");
        if (payloadV == nullptr)
        {
            std::cerr << "root value was missing the `payload` key\n";
            continue;
        }

        handler->second(metadata, *payloadV);
    }
}

awaitable<void> sessionWriter(WebSocketStream &ws)
{
    for (auto i = 0; i < 5; ++i)
    {
        std::cout << "done with writer\n";

        boost::asio::steady_timer timer(ws.get_executor());

        timer.expires_after(1s);

        auto [ec] = co_await timer.async_wait(use_nothrow_awaitable);

        if (ec)
        {
            fail(ec, "writer timer");
        }

        std::cout << "write\n";
        auto [writeError, _bytes_written] = co_await ws.async_write(
            net::buffer(std::string("forsen")), use_nothrow_awaitable);
        std::cout << "written\n";
        if (writeError)
        {
            fail(writeError, "write");
            continue;
        }
    }

    std::cout << "returned xd\n";
    co_return;
}

awaitable<WebSocketStream> session(WebSocketStream &&ws)
{
    // start reader
    std::cout << "start reader\n";
    co_await (sessionReader(ws));
    // co_spawn(ws.get_executor(), sessionReader(ws), detached);
    std::cout << "reader stopped\n";

    co_return ws;
}

awaitable<void> connectToClient(boost::asio::io_context &ioContext,
                                const std::string host, const std::string port,
                                const std::string path,
                                boost::asio::ssl::context &sslContext)
{
    auto tcpResolver = tcp::resolver(ioContext);

    for (;;)
    {
        // TODO: wait on (AND INCREMENT) backoff timer

        auto [resolveError, target] = co_await tcpResolver.async_resolve(
            host, port, use_nothrow_awaitable);

        std::cout << "Connecting to " << host << ":" << port << "\n";
        if (resolveError)
        {
            fail(resolveError, "resolve");
            continue;
        }

        WebSocketStream ws(ioContext, sslContext);

        // Make the connection on the IP address we get from a lookup
        auto [connectError, endpoint] =
            co_await beast::get_lowest_layer(ws).async_connect(
                target, use_nothrow_awaitable);

        std::string hostHeader = host;

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(),
                                      host.c_str()))
        {
            auto ec = beast::error_code(static_cast<int>(::ERR_get_error()),
                                        boost::asio::error::get_ssl_category());
            fail(ec, "connect");
            continue;
        }

        // Update the host string. This will provide the value of the
        // Host HTTP header during the WebSocket handshake.
        // See https://tools.ietf.org/html/rfc7230#section-5.4
        hostHeader += ':' + std::to_string(endpoint.port());

        // Set a timeout on the operation
        beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(30));

        // Set a decorator to change the User-Agent of the handshake
        ws.set_option(
            websocket::stream_base::decorator([](websocket::request_type &req) {
                req.set(http::field::user_agent,
                        std::string(BOOST_BEAST_VERSION_STRING) +
                            " websocket-client-coro");
            }));

        // Perform the SSL handshake
        auto [sslHandshakeError] = co_await ws.next_layer().async_handshake(
            ssl::stream_base::client, use_nothrow_awaitable);
        if (sslHandshakeError)
        {
            fail(sslHandshakeError, "ssl_handshake");
            continue;
        }

        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        beast::get_lowest_layer(ws).expires_never();

        // Set suggested timeout settings for the websocket
        ws.set_option(websocket::stream_base::timeout::suggested(
            beast::role_type::client));

        // Perform the websocket handshake
        auto [ws_handshake_ec] = co_await ws.async_handshake(
            hostHeader, path, use_nothrow_awaitable);
        if (ws_handshake_ec)
        {
            fail(ws_handshake_ec, "handshake");
            continue;
        }

        auto ws2 = co_await session(std::move(ws));

        // Close the WebSocket connection
        auto [closeError] = co_await ws2.async_close(
            websocket::close_code::normal, use_nothrow_awaitable);
        if (closeError)
        {
            fail(closeError, "close");
        }
        else
        {
            // If we get here then the connection is closed gracefully
            std::cout << "Closed connection gracefully\n";
        }

        // TODO: reset backoff timer
    }
}

int main(int argc, char **argv)
{
    try
    {
        boost::asio::io_context ctx;

        const auto *const host = "localhost";
        const auto *const port = "3012";
        const auto *const path = "/ws";

        boost::asio::ssl::context sslContext{
            boost::asio::ssl::context::tlsv12_client};

        // TODO: Load certificates into SSL context

        co_spawn(ctx, connectToClient(ctx, host, port, path, sslContext),
                 detached);

        ctx.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

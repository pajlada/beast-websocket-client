#include "eventsub/listener.hpp"
#include "eventsub/payloads/channel-ban-v1.hpp"
#include "eventsub/payloads/session-welcome.hpp"
#include "eventsub/session.hpp"

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

namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace http = beast::http;            // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace net = boost::asio;             // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;        // from <boost/asio/ssl.hpp>

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::ip::tcp;
using namespace boost::asio::experimental::awaitable_operators;
using namespace std::literals::chrono_literals;

using WebSocketStream = websocket::stream<beast::ssl_stream<beast::tcp_stream>>;

using namespace eventsub;

// Report a failure
void fail(beast::error_code ec, char const *what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

awaitable<WebSocketStream> session(WebSocketStream &&ws,
                                   std::unique_ptr<Listener> &&listener)
{
    // start reader
    std::cout << "start reader\n";
    co_await (sessionReader(ws, std::move(listener)));
    // co_spawn(ws.get_executor(), sessionReader(ws), detached);
    std::cout << "reader stopped\n";

    co_return ws;
}

class MyListener final : public Listener
{
public:
    void onSessionWelcome(messages::Metadata metadata,
                          payload::session_welcome::Payload payload) override
    {
        std::cout << "ON session welcome " << payload.id << " XD\n";
    }

    void onChannelBan(messages::Metadata metadata,
                      payload::channel_ban::v1::Payload payload) override
    {
        (void)metadata;
        std::cout << "Channel ban occured in "
                  << payload.event.broadcasterUserLogin << "'s channel:"
                  << " isPermanent=" << payload.event.isPermanent
                  << " reason=" << payload.event.reason
                  << " userLogin=" << payload.event.userLogin
                  << " moderatorLogin=" << payload.event.moderatorUserLogin
                  << '\n';
    }

    void onStreamOnline(messages::Metadata metadata,
                        payload::stream_online::v1::Payload payload) override
    {
        std::cout << "ON STREAM ONLINE XD\n";
    }

    void onStreamOffline(messages::Metadata metadata,
                         payload::stream_offline::v1::Payload payload) override
    {
        std::cout << "ON STREAM OFFLINE XD\n";
    }

    void onChannelChatNotification(
        messages::Metadata metadata,
        payload::channel_chat_notification::beta::Payload payload) override
    {
        std::cout << "Received channel.chat.notification beta\n";
    }

    void onChannelUpdate(messages::Metadata metadata,
                         payload::channel_update::v1::Payload payload) override
    {
        std::cout << "Channel update event!\n";
    }

    // Add your new subscription types above this line
};

awaitable<void> connectToClient(boost::asio::io_context &ioContext,
                                const std::string host, const std::string port,
                                const std::string path,
                                boost::asio::ssl::context &sslContext)
{
    auto tcpResolver = tcp::resolver(ioContext);

    for (;;)
    {
        // TODO: wait on (AND INCREMENT) backoff timer

        boost::system::error_code resolveError;
        auto target = co_await tcpResolver.async_resolve(
            host, port,
            boost::asio::redirect_error(boost::asio::use_awaitable,
                                        resolveError));

        std::cout << "Connecting to " << host << ":" << port << "\n";
        if (resolveError)
        {
            fail(resolveError, "resolve");
            continue;
        }

        WebSocketStream ws(ioContext, sslContext);

        // Make the connection on the IP address we get from a lookup
        // TODO: Check connectError
        boost::system::error_code connectError;
        auto endpoint = co_await beast::get_lowest_layer(ws).async_connect(
            target, boost::asio::redirect_error(boost::asio::use_awaitable,
                                                connectError));

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
        boost::system::error_code sslHandshakeError;
        co_await ws.next_layer().async_handshake(
            ssl::stream_base::client,
            boost::asio::redirect_error(boost::asio::use_awaitable,
                                        sslHandshakeError));
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
        boost::system::error_code wsHandshakeError;
        co_await ws.async_handshake(
            hostHeader, path,
            boost::asio::redirect_error(boost::asio::use_awaitable,
                                        wsHandshakeError));
        if (wsHandshakeError)
        {
            fail(wsHandshakeError, "handshake");
            continue;
        }

        std::unique_ptr<Listener> listener = std::make_unique<MyListener>();
        auto ws2 = co_await session(std::move(ws), std::move(listener));

        // Close the WebSocket connection
        boost::system::error_code closeError;
        co_await ws2.async_close(websocket::close_code::normal,
                                 boost::asio::redirect_error(
                                     boost::asio::use_awaitable, closeError));
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

        // for use with twitch CLI: twitch event websocket start-server --ssl --port 3012
        // const auto *const host = "localhost";
        // const auto *const port = "3012";
        // const auto *const path = "/ws";

        // for use with real Twitch eventsub
        const auto *const host = "eventsub.wss.twitch.tv";
        const auto *const port = "443";
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

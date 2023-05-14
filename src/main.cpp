#include <boost/asio.hpp>
#include <boost/asio/experimental/as_tuple.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include <array>
#include <chrono>
#include <iostream>
#include <memory>

namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace http = beast::http;            // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace net = boost::asio;             // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;        // from <boost/asio/ssl.hpp>
                                         //
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
    boost::asio::experimental::as_tuple(boost::asio::use_awaitable);

// Report a failure
void fail(beast::error_code ec, char const *what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

struct proxy_state {
    proxy_state(tcp::socket client)
        : client(std::move(client))
    {
    }

    tcp::socket client;
    tcp::socket server{client.get_executor()};
    steady_clock::time_point deadline;
};

using proxy_state_ptr = std::shared_ptr<proxy_state>;

awaitable<void> sessionReader(WebSocketStream &ws)
{
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

        std::cout << "read: " << beast::make_printable(buffer.data())
                  << std::endl;
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

awaitable<void> client_to_server(proxy_state_ptr state)
{
    std::array<char, 1024> data;

    for (;;)
    {
        state->deadline = std::max(state->deadline, steady_clock::now() + 5s);

        auto [e1, n1] = co_await state->client.async_read_some(
            buffer(data), use_nothrow_awaitable);
        if (e1)
            co_return;

        auto [e2, n2] = co_await async_write(state->server, buffer(data, n1),
                                             use_nothrow_awaitable);
        if (e2)
            co_return;
    }
}

awaitable<void> server_to_client(proxy_state_ptr state)
{
    std::array<char, 1024> data;

    for (;;)
    {
        state->deadline = std::max(state->deadline, steady_clock::now() + 5s);

        auto [e1, n1] = co_await state->server.async_read_some(
            buffer(data), use_nothrow_awaitable);
        if (e1)
            co_return;

        auto [e2, n2] = co_await async_write(state->client, buffer(data, n1),
                                             use_nothrow_awaitable);
        if (e2)
            co_return;
    }
}

awaitable<void> watchdog(proxy_state_ptr state)
{
    boost::asio::steady_timer timer(state->client.get_executor());

    auto now = steady_clock::now();
    while (state->deadline > now)
    {
        timer.expires_at(state->deadline);
        co_await timer.async_wait(use_nothrow_awaitable);
        now = steady_clock::now();
    }
}

awaitable<void> proxy(tcp::socket client, tcp::endpoint target)
{
    auto state = std::make_shared<proxy_state>(std::move(client));

    auto [e] =
        co_await state->server.async_connect(target, use_nothrow_awaitable);
    if (!e)
    {
        co_await (client_to_server(state) || server_to_client(state) ||
                  watchdog(state));

        state->client.close();
        state->server.close();
    }
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

        boost::asio::ssl::context sslContext{
            boost::asio::ssl::context::tlsv12_client};

        const auto *const host = "localhost";
        const auto *const port = "3012";
        const auto *const path = "/ws";

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

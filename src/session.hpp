#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

namespace eventsub {

class Listener;

using WebSocketStream = boost::beast::websocket::stream<
    boost::beast::ssl_stream<boost::beast::tcp_stream>>;

boost::asio::awaitable<void> sessionReader(WebSocketStream &ws,
                                           std::unique_ptr<Listener> listener);

}  // namespace eventsub

#pragma once

#include "messages/errors.hpp"

#include <boost/json.hpp>

#include <string>

namespace twitch::eventsub {

/*
{
  "metadata": ...
  "payload": {
    "session": {
      "id": "44f8cbce_c7ee958a",
      "status": "connected",
      "keepalive_timeout_seconds": 10,
      "reconnect_url": null,
      "connected_at": "2023-05-14T12:31:47.995262791Z"
    }
  }
}
*/

struct SessionWelcome {
    const std::string id;

    SessionWelcome(std::string_view _id)
        : id(_id)
    {
    }
};

boost::json::result_for<SessionWelcome, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<SessionWelcome>,
    const boost::json::value &payloadV)
{
    if (!payloadV.is_object())
    {
        return boost::system::error_code{129, error::EXPECTED_OBJECT};
    }
    const auto &payload = payloadV.get_object();

    const auto *sessionV = payload.if_contains("session");
    if (sessionV == nullptr)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    const auto *session = sessionV->if_object();
    if (session == nullptr)
    {
        return boost::system::error_code{129, error::EXPECTED_OBJECT};
    }

    const auto *idV = session->if_contains("id");
    if (idV == nullptr)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }

    const auto id = boost::json::try_value_to<std::string>(*idV);
    if (id.has_error())
    {
        return id.error();
    }

    return SessionWelcome{
        id.value(),
    };
}

}  // namespace twitch::eventsub

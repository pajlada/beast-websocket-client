#pragma once

#include "helpers.hpp"
#include "messages/errors.hpp"

#include <boost/json.hpp>

#include <string>

namespace eventsub::payload::session_welcome {

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

struct Payload {
    const std::string id;
};

boost::json::result_for<Payload, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Payload>, const boost::json::value &payloadV)
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
    if (!sessionV->is_object())
    {
        return boost::system::error_code{129, error::EXPECTED_OBJECT};
    }
    const auto &session = sessionV->get_object();

    const auto id = readMember<std::string>(session, "id");
    if (!id)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }

    return Payload{
        .id = *id,
    };
}

}  // namespace eventsub::payload::session_welcome

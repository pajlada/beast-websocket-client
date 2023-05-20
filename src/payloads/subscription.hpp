#pragma once

#include "helpers.hpp"
#include "messages/errors.hpp"
#include "payloads/subscription.hpp"

#include <boost/json.hpp>

#include <string>

namespace eventsub::payload::subscription {

/*
{
  "metadata": ...,
  "payload": {
    "subscription": {
      "id": "4aa632e0-fca3-590b-e981-bbd12abdb3fe",
      "status": "enabled",
      "type": "channel.ban",
      "version": "1",
      "condition": {
        "broadcaster_user_id": "74378979"
      },
      "transport": {
        "method": "websocket",
        "session_id": "38de428e_b11f07be"
      },
      "created_at": "2023-05-20T12:30:55.518375571Z",
      "cost": 0
    },
    ...
  }
}
*/

struct Transport {
    const std::string method;
    const std::string sessionID;
};

boost::json::result_for<Transport, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Transport>, const boost::json::value &jv)
{
    if (!jv.is_object())
    {
        return boost::system::error_code{129, error::EXPECTED_OBJECT};
    }
    const auto &root = jv.get_object();

    auto method = readMember<std::string>(root, "method");
    if (!method)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    auto sessionID = readMember<std::string>(root, "session_id");
    if (!sessionID)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }

    return Transport{
        .method = *method,
        .sessionID = *sessionID,
    };
}

struct Subscription {
    const std::string id;
    const std::string status;
    const std::string type;
    const std::string version;

    // TODO: How do we map condition here? vector of key/value pairs?

    const Transport transport;

    // TODO: chronofy?
    const std::string createdAt;
    const int cost;
};

boost::json::result_for<Subscription, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Subscription>,
    const boost::json::value &rootV)
{
    if (!rootV.is_object())
    {
        return boost::system::error_code{129, error::EXPECTED_OBJECT};
    }
    const auto &root = rootV.get_object();

    auto id = readMember<std::string>(root, "id");
    if (!id)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    auto status = readMember<std::string>(root, "status");
    if (!status)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    auto type = readMember<std::string>(root, "type");
    if (!type)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    auto version = readMember<std::string>(root, "version");
    if (!version)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    auto transport = readMember<Transport>(root, "transport");
    if (!transport)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    auto createdAt = readMember<std::string>(root, "created_at");
    if (!createdAt)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    auto cost = readMember<int>(root, "cost");
    if (!cost)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }

    return Subscription{
        .id = *id,
        .status = *status,
        .type = *type,
        .version = *version,
        .transport = *transport,
        .createdAt = *createdAt,
        .cost = *cost,
    };
}

}  // namespace eventsub::payload::subscription

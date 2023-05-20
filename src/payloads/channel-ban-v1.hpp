#pragma once

#include "helpers.hpp"
#include "messages/errors.hpp"
#include "payloads/subscription.hpp"

#include <boost/json.hpp>

#include <string>

namespace eventsub::payload::channel_ban::v1 {

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
    "event": {
      "banned_at": "2023-05-20T12:30:55.518375571Z",
      "broadcaster_user_id": "74378979",
      "broadcaster_user_login": "testBroadcaster",
      "broadcaster_user_name": "testBroadcaster",
      "ends_at": "2023-05-20T12:40:55.518375571Z",
      "is_permanent": false,
      "moderator_user_id": "29024944",
      "moderator_user_login": "CLIModerator",
      "moderator_user_name": "CLIModerator",
      "reason": "This is a test event",
      "user_id": "40389552",
      "user_login": "testFromUser",
      "user_name": "testFromUser"
    }
  }
}
*/

struct Event {
    const std::string bannedAt;
    const std::string broadcasterUserID;
    const std::string broadcasterUserLogin;
    const std::string broadcasterUserName;
    // TODO: chronofy?
    const std::string endsAt;
    const bool isPermanent;
    const std::string moderatorUserID;
    const std::string moderatorUserLogin;
    const std::string moderatorUserName;
    const std::string reason;
    const std::string userID;
    const std::string userLogin;
    const std::string userName;
};

boost::json::result_for<Event, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Event>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        return boost::system::error_code{129, error::EXPECTED_OBJECT};
    }
    const auto &root = jvRoot.get_object();

    const auto bannedAt = readMember<std::string>(root, "banned_at");
    if (!bannedAt)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    const auto broadcasterUserID =
        readMember<std::string>(root, "broadcaster_user_id");
    if (!broadcasterUserID)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    const auto broadcasterUserLogin =
        readMember<std::string>(root, "broadcaster_user_login");
    if (!broadcasterUserLogin)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    const auto broadcasterUserName =
        readMember<std::string>(root, "broadcaster_user_name");
    if (!broadcasterUserName)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    const auto endsAt = readMember<std::string>(root, "ends_at");
    if (!endsAt)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    const auto isPermanent = readMember<bool>(root, "is_permanent");
    if (!isPermanent)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    const auto moderatorUserID =
        readMember<std::string>(root, "moderator_user_id");
    if (!moderatorUserID)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    const auto moderatorUserLogin =
        readMember<std::string>(root, "moderator_user_login");
    if (!moderatorUserLogin)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    const auto moderatorUserName =
        readMember<std::string>(root, "moderator_user_name");
    if (!moderatorUserName)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    const auto reason = readMember<std::string>(root, "reason");
    if (!reason)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    const auto userID = readMember<std::string>(root, "user_id");
    if (!userID)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    const auto userLogin = readMember<std::string>(root, "user_login");
    if (!userLogin)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    const auto userName = readMember<std::string>(root, "user_name");
    if (!userName)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }

    return Event{
        .bannedAt = *bannedAt,
        .broadcasterUserID = *broadcasterUserID,
        .broadcasterUserLogin = *broadcasterUserLogin,
        .broadcasterUserName = *broadcasterUserName,
        .endsAt = *endsAt,
        .isPermanent = *isPermanent,
        .moderatorUserID = *moderatorUserID,
        .moderatorUserLogin = *moderatorUserLogin,
        .moderatorUserName = *moderatorUserName,
        .reason = *reason,
        .userID = *userID,
        .userLogin = *userLogin,
        .userName = *userName,
    };
}

struct Payload {
    const subscription::Subscription subscription;

    const Event event;
};

boost::json::result_for<Payload, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Payload>, const boost::json::value &payloadV)
{
    if (!payloadV.is_object())
    {
        return boost::system::error_code{129, error::EXPECTED_OBJECT};
    }
    const auto &payload = payloadV.get_object();

    const auto subscription =
        readMember<subscription::Subscription>(payload, "subscription");
    if (!subscription)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    const auto event = readMember<Event>(payload, "event");
    if (!event)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }

    return Payload{
        .subscription = *subscription,
        .event = *event,
    };
}

}  // namespace eventsub::payload::channel_ban::v1

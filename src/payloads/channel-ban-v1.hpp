#pragma once

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

/// json_transform=snake_case
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

struct Payload {
    const subscription::Subscription subscription;

    const Event event;
};

// DESERIALIZATION DEFINITION START
boost::json::result_for<Event, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Event>, const boost::json::value &jvRoot);

boost::json::result_for<Payload, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Payload>, const boost::json::value &jvRoot);
// DESERIALIZATION DEFINITION END

}  // namespace eventsub::payload::channel_ban::v1

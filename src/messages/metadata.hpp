#pragma once

#include "messages/errors.hpp"

#include <boost/json.hpp>

#include <string>

namespace twitch::eventsub {

/*
{
  "metadata": {
    "message_id": "40cc68b8-dc5b-a46e-0388-a7c9193eec5e",
    "message_type": "session_welcome",
    "message_timestamp": "2023-05-14T12:31:47.995298776Z"
  },
  "payload": ...
}
*/

struct Metadata {
    const std::string messageID;
    const std::string messageType;
    // TODO: should this be chronofied?
    const std::string messageTimestamp;

    Metadata(std::string_view _messageID, std::string_view _messageType,
             std::string_view _messageTimestamp)
        : messageID(_messageID)
        , messageType(_messageType)
        , messageTimestamp(_messageTimestamp)
    {
    }
};

boost::json::result_for<Metadata, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Metadata>, const boost::json::value &rootV)
{
    if (!rootV.is_object())
    {
        return boost::system::error_code{129, error::EXPECTED_OBJECT};
    }
    const auto &root = rootV.get_object();

    const auto *messageIDV = root.if_contains("message_id");
    if (messageIDV == nullptr)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    const auto messageID = boost::json::try_value_to<std::string>(*messageIDV);
    if (messageID.has_error())
    {
        return messageID.error();
    }

    const auto *messageTypeV = root.if_contains("message_type");
    if (messageTypeV == nullptr)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    const auto messageType =
        boost::json::try_value_to<std::string>(*messageTypeV);
    if (messageType.has_error())
    {
        return messageType.error();
    }

    const auto *messageTimestampV = root.if_contains("message_timestamp");
    if (messageTimestampV == nullptr)
    {
        return boost::system::error_code{129, error::MISSING_KEY};
    }
    const auto messageTimestamp =
        boost::json::try_value_to<std::string>(*messageTimestampV);
    if (messageTimestamp.has_error())
    {
        return messageTimestamp.error();
    }

    return Metadata{
        messageID.value(),
        messageType.value(),
        messageTimestamp.value(),
    };
}

}  // namespace twitch::eventsub

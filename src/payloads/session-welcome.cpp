#include "eventsub/payloads/session-welcome.hpp"

#include "eventsub/errors.hpp"

#include <boost/json.hpp>

namespace eventsub::payload::session_welcome {

// DESERIALIZATION IMPLEMENTATION START
boost::json::result_for<Payload, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Payload>, const boost::json::value &jvRoot)
{
    if (!jvRoot.is_object())
    {
        return boost::system::error_code{129, error::EXPECTED_OBJECT};
    }
    const auto &outerRoot = jvRoot.get_object();

    const auto *jvInnerRoot = outerRoot.if_contains("session");
    if (jvInnerRoot == nullptr)
    {
        return boost::system::error_code{129, error::EXPECTED_OBJECT};
    }
    if (!jvInnerRoot->is_object())
    {
        return boost::system::error_code{129, error::EXPECTED_OBJECT};
    }
    const auto &root = jvInnerRoot->get_object();

    const auto *jvid = root.if_contains("id");
    if (jvid == nullptr)
    {
        static const error::ApplicationErrorCategory error_missing_field_id{
            "Missing required key id"};
        return boost::system::error_code{129, error_missing_field_id};
    }
    const auto id = boost::json::try_value_to<std::string>(*jvid);
    if (id.has_error())
    {
        return id.error();
    }

    return Payload{
        .id = id.value(),
    };
}
// DESERIALIZATION IMPLEMENTATION END

}  // namespace eventsub::payload::session_welcome

/**
 * @file fileLocationSubscription.h
 * @author Krzysztof Trzepla
 * @copyright (C) 2015 ACK CYFRONET AGH
 * @copyright This software is released under the MIT license cited in
 * 'LICENSE.txt'
 */

#ifndef ONECLIENT_EVENTS_SUBSCRIPTIONS_FILE_LOCATION_SUBSCRIPTION_H
#define ONECLIENT_EVENTS_SUBSCRIPTIONS_FILE_LOCATION_SUBSCRIPTION_H

#include "messages/clientMessage.h"
#include "subscription.h"

#include <boost/optional.hpp>

#include <chrono>
#include <cstddef>

namespace one {
namespace client {
namespace events {

/**
 * @c FileLocationSubscription is a client side subscription and represents a
 * request for file location updates.
 */
class FileLocationSubscription : public Subscription,
                                 public messages::ClientMessage {
public:
    /**
     * Constructor.
     * @param fileUuid UUID of file for which Location updates are
     * requested.
     * @param counterThreshold Maximal number of aggregated events before
     * emission.
     * @param timeThreshold Maximal delay in milliseconds between successive
     * events emissions.
     */
    FileLocationSubscription(std::string fileUuid,
        boost::optional<std::size_t> counterThreshold = {},
        boost::optional<std::chrono::milliseconds> timeThreshold = {});

    std::string toString() const override;

private:
    std::unique_ptr<one::messages::ProtocolClientMessage>
    serializeAndDestroy() override;

    std::string m_fileUuid;
};

} // namespace events
} // namespace client
} // namespace one

#endif // ONECLIENT_EVENTS_SUBSCRIPTIONS_FILE_LOCATION_SUBSCRIPTION_H

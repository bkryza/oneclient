/**
 * @file eventStream.h
 * @author Krzysztof Trzepla
 * @copyright (C) 2015 ACK CYFRONET AGH
 * @copyright This software is released under the MIT license cited in
 * 'LICENSE.txt'
 */

#ifndef ONECLIENT_EVENTS_EVENT_STREAM_H
#define ONECLIENT_EVENTS_EVENT_STREAM_H

#include "events/aggregators/eventCounterAggregator.h"
#include "events/aggregators/eventSizeAggregator.h"
#include "events/aggregators/eventTimeAggregator.h"
#include "events/eventCommunicator.h"
#include "events/eventHandler.h"
#include "events/eventWorker.h"
#include "events/subscriptionHandler.h"
#include "events/subscriptions/fileAccessedSubscription.h"
#include "events/subscriptions/fileAttrSubscription.h"
#include "events/subscriptions/fileLocationSubscription.h"
#include "events/subscriptions/fileRemovalSubscription.h"
#include "events/subscriptions/fileRenamedSubscription.h"
#include "events/subscriptions/permissionChangedSubscription.h"
#include "events/subscriptions/quotaSubscription.h"
#include "events/subscriptions/readSubscription.h"
#include "events/subscriptions/writeSubscription.h"
#include "events/types/fileAccessedEvent.h"
#include "events/types/fileRemovalEvent.h"
#include "events/types/fileRenamedEvent.h"
#include "events/types/permissionChangedEvent.h"
#include "events/types/quotaExeededEvent.h"
#include "events/types/readEvent.h"
#include "events/types/updateEvent.h"
#include "events/types/writeEvent.h"
#include "events/types/permissionChangedEvent.h"
#include "messages/fuse/fileAttr.h"
#include "messages/fuse/fileLocation.h"

namespace one {
namespace client {
namespace events {

using ReadEventStream =
    EventWorker<EventCounterAggregator<EventSizeAggregator<EventTimeAggregator<
        SubscriptionHandler<EventHandler<EventCommunicator<ReadEvent>>>>>>>;

using WriteEventStream =
    EventWorker<EventCounterAggregator<EventSizeAggregator<EventTimeAggregator<
        SubscriptionHandler<EventHandler<EventCommunicator<WriteEvent>>>>>>>;

using FileAttrEventStream = EventWorker<
    EventCounterAggregator<EventTimeAggregator<SubscriptionHandler<EventHandler<
        EventCommunicator<UpdateEvent<messages::fuse::FileAttr>>>>>>>;

using FileLocationEventStream = EventWorker<
    EventCounterAggregator<EventTimeAggregator<SubscriptionHandler<EventHandler<
        EventCommunicator<UpdateEvent<messages::fuse::FileLocation>>>>>>>;

using PermissionChangedEventStream =
    EventWorker<EventCounterAggregator<EventTimeAggregator<SubscriptionHandler<
        EventHandler<EventCommunicator<PermissionChangedEvent>>>>>>;

using FileRemovalEventStream =
    EventWorker<EventCounterAggregator<EventTimeAggregator<SubscriptionHandler<
        EventHandler<EventCommunicator<FileRemovalEvent>>>>>>;

using QuotaExeededEventStream =
    EventWorker<EventCounterAggregator<EventTimeAggregator<SubscriptionHandler<
        EventHandler<EventCommunicator<QuotaExeededEvent>>>>>>;

using FileRenamedEventStream =
    EventWorker<EventCounterAggregator<EventTimeAggregator<SubscriptionHandler<
        EventHandler<EventCommunicator<FileRenamedEvent>>>>>>;

using FileAccessedEventStream =
    EventWorker<EventCounterAggregator<EventTimeAggregator<SubscriptionHandler<
        EventHandler<EventCommunicator<FileAccessedEvent>>>>>>;

} // namespace events
} // namespace client
} // namespace one

#endif // ONECLIENT_EVENTS_EVENT_STREAM_H

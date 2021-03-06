/**
 * @file eventManager.cc
 * @author Krzysztof Trzepla
 * @copyright (C) 2015 ACK CYFRONET AGH
 * @copyright This software is released under the MIT license cited in
 * 'LICENSE.txt'
 */

#include "eventManager.h"

#include "communication/subscriptionData.h"
#include "context.h"
#include "scheduler.h"
#include "subscriptionRegistry.h"
#include "subscriptions/subscriptionCancellation.h"

#include "messages.pb.h"

namespace one {
namespace client {
namespace events {

EventManager::EventManager(std::shared_ptr<Context> context)
    : m_streamManager{context->communicator()}
    , m_registry{std::make_shared<SubscriptionRegistry>()}
    , m_readEventStream{std::make_unique<ReadEventStream>(
          m_streamManager.create())}
    , m_writeEventStream{std::make_unique<WriteEventStream>(
          m_streamManager.create())}
    , m_fileAttrEventStream{std::make_unique<FileAttrEventStream>(
          m_streamManager.create())}
    , m_fileLocationEventStream{std::make_unique<FileLocationEventStream>(
          m_streamManager.create())}
    , m_permissionChangedEventStream{std::make_unique<
          PermissionChangedEventStream>(m_streamManager.create())}
    , m_fileRemovalEventStream{std::make_unique<FileRemovalEventStream>(
          m_streamManager.create())}
    , m_quotaExeededEventStream{
          std::make_unique<QuotaExeededEventStream>(m_streamManager.create())}
    , m_fileRenamedEventStream{
          std::make_unique<FileRenamedEventStream>(m_streamManager.create())}
    , m_fileAccessedEventStream{
          std::make_unique<FileAccessedEventStream>(m_streamManager.create())}

{
    auto predicate = [](const clproto::ServerMessage &message, const bool) {
        return message.has_events() || message.has_subscription() ||
            message.has_subscription_cancellation();
    };
    auto callback = [this](const clproto::ServerMessage &message) {
        if (message.has_events())
            handle(message.events());
        else if (message.has_subscription())
            handle(message.subscription());
        else if (message.has_subscription_cancellation())
            handle(message.subscription_cancellation());
    };
    context->communicator()->subscribe(communication::SubscriptionData{
        std::move(predicate), std::move(callback)});

    initializeStreams(std::move(context));
}

void EventManager::emitReadEvent(
    off_t offset, size_t size, std::string fileUuid) const
{
    m_readEventStream->createAndEmitEvent(offset, size, std::move(fileUuid));
}

void EventManager::emitWriteEvent(off_t offset, std::size_t size,
    std::string fileUuid, std::string storageId, std::string fileId) const
{
    m_writeEventStream->createAndEmitEvent(offset, size, std::move(fileUuid),
        std::move(storageId), std::move(fileId));
}

void EventManager::emitTruncateEvent(off_t fileSize, std::string fileUuid) const
{
    m_writeEventStream->createAndEmitEvent(0, 0, fileSize, std::move(fileUuid));
}

void EventManager::setFileAttrHandler(FileAttrEventStream::Handler handler)
{
    m_fileAttrEventStream->setEventHandler(std::move(handler));
}

std::int64_t EventManager::subscribe(FileAttrSubscription clientSubscription,
    FileAttrSubscription serverSubscription)
{
    return m_fileAttrEventStream->subscribe(
        std::move(clientSubscription), std::move(serverSubscription));
}

void EventManager::setFileLocationHandler(
    FileLocationEventStream::Handler handler)
{
    m_fileLocationEventStream->setEventHandler(std::move(handler));
}

void EventManager::setFileRemovalHandler(
    FileRemovalEventStream::Handler handler)
{
    m_fileRemovalEventStream->setEventHandler(std::move(handler));
}

void EventManager::setQuotaExeededHandler(
    QuotaExeededEventStream::Handler handler)
{
    m_quotaExeededEventStream->setEventHandler(std::move(handler));
}

void EventManager::setFileRenamedHandler(
    FileRenamedEventStream::Handler handler)
{
    m_fileRenamedEventStream->setEventHandler(std::move(handler));
}

void EventManager::emitFileRemovalEvent(std::string fileUuid) const
{
    m_fileRemovalEventStream->createAndEmitEvent(std::move(fileUuid));
}

std::int64_t EventManager::subscribe(FileRemovalSubscription clientSubscription,
    FileRemovalSubscription serverSubscription)
{
    return m_fileRemovalEventStream->subscribe(
        std::move(clientSubscription), std::move(serverSubscription));
}

std::int64_t EventManager::subscribe(FileRenamedSubscription clientSubscription,
    FileRenamedSubscription serverSubscription)
{
    return m_fileRenamedEventStream->subscribe(
        std::move(clientSubscription), std::move(serverSubscription));
}

void EventManager::emitFileOpenedEvent(std::string fileUuid) const
{
    m_fileAccessedEventStream->createAndEmitEvent(std::move(fileUuid), 1, 0);
}

void EventManager::emitFileReleasedEvent(std::string fileUuid) const
{
    m_fileAccessedEventStream->createAndEmitEvent(std::move(fileUuid), 0, 1);
}

std::int64_t EventManager::subscribe(
    FileLocationSubscription clientSubscription,
    FileLocationSubscription serverSubscription)
{
    return m_fileLocationEventStream->subscribe(
        std::move(clientSubscription), std::move(serverSubscription));
}

std::int64_t EventManager::subscribe(
    QuotaSubscription clientSubscription, QuotaSubscription serverSubscription)
{
    return m_quotaExeededEventStream->subscribe(
        std::move(clientSubscription), std::move(serverSubscription));
}

void EventManager::setPermissionChangedHandler(
    PermissionChangedEventStream::Handler handler)
{
    m_permissionChangedEventStream->setEventHandler(std::move(handler));
}

std::int64_t EventManager::subscribe(
    PermissionChangedSubscription clientSubscription,
    PermissionChangedSubscription serverSubscription)
{
    return m_permissionChangedEventStream->subscribe(
        std::move(clientSubscription), std::move(serverSubscription));
}

void EventManager::subscribe(SubscriptionContainer container)
{
    auto readSubscriptions = container.moveReadSubscriptions();
    for (auto &subscription : readSubscriptions)
        m_readEventStream->subscribe(std::move(subscription));

    auto writeSubscriptions = container.moveWriteSubscriptions();
    for (auto &subscription : writeSubscriptions)
        m_writeEventStream->subscribe(std::move(subscription));

    auto fileAccessedSubscription = container.moveFileAccessedSubscription();
    for (auto &subscription : fileAccessedSubscription)
        m_fileAccessedEventStream->subscribe(std::move(subscription));
}

bool EventManager::unsubscribe(std::int64_t id)
{
    return m_registry->removeSubscription(SubscriptionCancellation{id});
}

void EventManager::handle(const clproto::Events &message)
{
    using namespace messages::fuse;

    for (const auto &eventMsg : message.events()) {
        if (eventMsg.has_update_event()) {
            const auto &updateEvent = eventMsg.update_event();
            if (updateEvent.has_file_attr()) {
                UpdateEvent<FileAttr> event{updateEvent.file_attr()};
                m_fileAttrEventStream->emitEvent(std::move(event));
            }
            else if (updateEvent.has_file_location()) {
                UpdateEvent<FileLocation> event{updateEvent.file_location()};
                m_fileLocationEventStream->emitEvent(std::move(event));
            }
        }
        if (eventMsg.has_permission_changed_event()) {
            PermissionChangedEvent event{eventMsg.permission_changed_event()};
            m_permissionChangedEventStream->emitEvent(std::move(event));
        }
        if (eventMsg.has_file_removal_event()) {
            FileRemovalEvent event{eventMsg.file_removal_event()};
            m_fileRemovalEventStream->emitEvent(std::move(event));
        }
        if (eventMsg.has_quota_exeeded_event()) {
            QuotaExeededEvent event{eventMsg.quota_exeeded_event()};
            m_quotaExeededEventStream->emitEvent(std::move(event));
        }
        if (eventMsg.has_file_renamed_event()) {
            FileRenamedEvent event{eventMsg.file_renamed_event()};
            m_fileRenamedEventStream->emitEvent(std::move(event));
        }
    }
}

void EventManager::handle(const clproto::Subscription &message)
{
    const auto id = message.id();
    if (message.has_read_subscription()) {
        ReadSubscription subscription{id, message.read_subscription()};
        m_readEventStream->subscribe(std::move(subscription));
    }
    else if (message.has_write_subscription()) {
        WriteSubscription subscription{id, message.write_subscription()};
        m_writeEventStream->subscribe(std::move(subscription));
    }
    else if (message.has_file_accessed_subscription()) {
        FileAccessedSubscription subscription{id};
        m_fileAccessedEventStream->subscribe(std::move(subscription));
    }
}

void EventManager::handle(const clproto::SubscriptionCancellation &message)
{
    SubscriptionCancellation cancellation{message};
    m_registry->removeSubscription(cancellation);
}

std::shared_ptr<SubscriptionRegistry> EventManager::subscriptionRegistry() const
{
    return m_registry;
}

void EventManager::initializeStreams(std::shared_ptr<Context> context)
{
    m_readEventStream->setScheduler(context->scheduler());
    m_readEventStream->setSubscriptionRegistry(m_registry);
    m_readEventStream->setEventHandler([this](auto events) {
        m_readEventStream->communicator.send(std::move(events));
    });
    m_readEventStream->initializeAggregation();

    m_writeEventStream->setScheduler(context->scheduler());
    m_writeEventStream->setSubscriptionRegistry(m_registry);
    m_writeEventStream->setEventHandler([this](auto events) {
        m_writeEventStream->communicator.send(std::move(events));
    });
    m_writeEventStream->initializeAggregation();

    m_fileAttrEventStream->setScheduler(context->scheduler());
    m_fileAttrEventStream->setSubscriptionRegistry(m_registry);
    m_fileAttrEventStream->initializeAggregation();

    m_fileLocationEventStream->setScheduler(context->scheduler());
    m_fileLocationEventStream->setSubscriptionRegistry(m_registry);
    m_fileLocationEventStream->initializeAggregation();

    m_permissionChangedEventStream->setScheduler(context->scheduler());
    m_permissionChangedEventStream->setSubscriptionRegistry(m_registry);
    m_permissionChangedEventStream->initializeAggregation();

    m_fileRemovalEventStream->setScheduler(context->scheduler());
    m_fileRemovalEventStream->setSubscriptionRegistry(m_registry);
    m_fileRemovalEventStream->initializeAggregation();

    m_quotaExeededEventStream->setScheduler(context->scheduler());
    m_quotaExeededEventStream->setSubscriptionRegistry(m_registry);
    m_quotaExeededEventStream->initializeAggregation();

    m_fileRenamedEventStream->setSubscriptionRegistry(m_registry);
    m_fileRenamedEventStream->initializeAggregation();

    m_fileAccessedEventStream->setScheduler(context->scheduler());
    m_fileAccessedEventStream->setSubscriptionRegistry(m_registry);
    m_fileAccessedEventStream->setEventHandler([this](auto events) {
        m_fileAccessedEventStream->communicator.send(std::move(events));
    });
    m_fileAccessedEventStream->initializeAggregation();
}

} // namespace events
} // namespace client
} // namespace one

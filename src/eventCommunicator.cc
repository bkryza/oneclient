/**
 * @file eventCommunicator.cc
 * @author Michal Sitko
 * @copyright (C) 2014 ACK CYFRONET AGH
 * @copyright This software is released under the MIT license cited in 'LICENSE.txt'
 */

#include "events/eventCommunicator.h"
#include "veilfs.h"
#include "communication_protocol.pb.h"

#include <boost/algorithm/string/predicate.hpp>
#include <google/protobuf/descriptor.h>

using namespace veil::client;
using namespace veil::client::events;
using namespace std;
using namespace boost;
using namespace veil::protocol::fuse_messages;
using namespace veil::protocol::communication_protocol;

EventCommunicator::EventCommunicator(boost::shared_ptr<EventStreamCombiner> eventsStream) : m_eventsStream(eventsStream), m_writeEnabled(true)
{
    if(!eventsStream){
        m_eventsStream = boost::shared_ptr<EventStreamCombiner>(new EventStreamCombiner());
    }
    m_messageBuilder.reset(new MessageBuilder());
}

void EventCommunicator::handlePushedConfig(const Answer &msg)
{
    EventStreamConfig eventStreamConfig;
    if(eventStreamConfig.ParseFromString(msg.worker_answer())){
        addEventSubstreamFromConfig(eventStreamConfig);
    }else{
        LOG(WARNING) << "Cannot parse pushed message as " << eventStreamConfig.GetDescriptor()->name();
    }
}

void EventCommunicator::handlePushedAtom(const Answer &msg)
{
    Atom atom;
    if(atom.ParseFromString(msg.worker_answer())){
        if(atom.value() == "write_enabled"){
            m_writeEnabled = true;
            LOG(INFO) << "writeEnabled true";
        }else if(atom.value() == "write_disabled"){
            m_writeEnabled = false;
            LOG(INFO) << "writeEnabled false";
        }else if(atom.value() == "test_atom2"){
            // just for test purposes
            // do nothing
        }else if(atom.value() == "test_atom2_ack" && msg.has_message_id() && msg.message_id() < -1){
            // just for test purposes
            PushListener::sendPushMessageAck("rule_manager", msg.message_id());
        }
    }else{
        LOG(WARNING) << "Cannot parse pushed message as " << atom.GetDescriptor()->name();
    }
}

bool EventCommunicator::pushMessagesHandler(const protocol::communication_protocol::Answer &msg)
{
    string messageType = msg.message_type();

    if(boost::iequals(messageType, EventStreamConfig::descriptor()->name())){
        handlePushedConfig(msg);
    }else if(boost::iequals(messageType, Atom::descriptor()->name())){
        handlePushedAtom(msg);
    }

    return true;
}

void EventCommunicator::configureByCluster()
{
    Atom atom;
    atom.set_value(EVENT_PRODUCER_CONFIG_REQUEST);

    ClusterMsg clm = m_messageBuilder->createClusterMessage(RULE_MANAGER, ATOM, COMMUNICATION_PROTOCOL, EVENT_PRODUCER_CONFIG, FUSE_MESSAGES, true, atom.SerializeAsString());

    boost::shared_ptr<CommunicationHandler> connection = VeilFS::getConnectionPool()->selectConnection();

    Answer ans;
    if(!connection || (ans=connection->communicate(clm, 0)).answer_status() == VEIO) {
        LOG(WARNING) << "sending atom eventproducerconfigrequest failed: " << (connection ? "failed" : "not needed");
    } else {
        VeilFS::getConnectionPool()->releaseConnection(connection);
        LOG(INFO) << "atom eventproducerconfigrequest sent";
    }

    LOG(INFO) << "eventproducerconfigrequest answer_status: " << ans.answer_status();

    EventProducerConfig config;
    if(!config.ParseFromString(ans.worker_answer())){
        LOG(WARNING) << "Cannot parse eventproducerconfigrequest answer as EventProducerConfig";
        return;
    }

    LOG(INFO) << "Fetched EventProducerConfig contains " << config.event_streams_configs_size() << " stream configurations";

    for(int i=0; i<config.event_streams_configs_size(); ++i)
    {
        addEventSubstreamFromConfig(config.event_streams_configs(i));
    }
}

void EventCommunicator::sendEvent(boost::shared_ptr<EventMessage> eventMessage)
{
    string encodedEventMessage = eventMessage->SerializeAsString();

    MessageBuilder messageBuilder;
    ClusterMsg clm = messageBuilder.createClusterMessage(CLUSTER_RENGINE, EVENT_MESSAGE, FUSE_MESSAGES, ATOM, COMMUNICATION_PROTOCOL, false, encodedEventMessage);

    boost::shared_ptr<CommunicationHandler> connection = VeilFS::getConnectionPool()->selectConnection();

    Answer ans;
    if(!connection || (ans=connection->communicate(clm, 0)).answer_status() == VEIO) {
        LOG(WARNING) << "sending event message failed";
    } else {
        VeilFS::getConnectionPool()->releaseConnection(connection);
        DLOG(INFO) << "Event message sent";
    }
}

bool EventCommunicator::askClusterIfWriteEnabled()
{
    Atom atom;
    atom.set_value("is_write_enabled");

    ClusterMsg clm = m_messageBuilder->createClusterMessage(FSLOGIC, ATOM, COMMUNICATION_PROTOCOL, ATOM, COMMUNICATION_PROTOCOL, true, atom.SerializeAsString());

    boost::shared_ptr<CommunicationHandler> connection = VeilFS::getConnectionPool()->selectConnection();

    Answer ans;
    if(!connection || (ans=connection->communicate(clm, 0)).answer_status() == VEIO) {
        LOG(WARNING) << "sending atom is_write_enabled failed";
    } else {
        VeilFS::getConnectionPool()->releaseConnection(connection);
        LOG(INFO) << "atom is_write_enabled sent";
    }

    Atom response;
    bool result;
    if(!response.ParseFromString(ans.worker_answer())){
        result = true;
        LOG(WARNING) << " cannot parse is_write_enabled response as atom. Using WriteEnabled = true mode.";
    }else{
        result = response.value() == "false" ? false : true;
    }

    return result;
}

void EventCommunicator::addEventSubstream(boost::shared_ptr<IEventStream> newStream)
{
    AutoLock lock(m_eventsStreamLock, WRITE_LOCK);
    m_eventsStream->addSubstream(newStream);
    LOG(INFO) << "New EventStream added to EventCommunicator.";
}

void EventCommunicator::addEventSubstreamFromConfig(const EventStreamConfig & eventStreamConfig)
{
    boost::shared_ptr<IEventStream> newStream = IEventStreamFactory::fromConfig(eventStreamConfig);
    if(newStream){
        addEventSubstream(newStream);
    }
}

void EventCommunicator::processEvent(boost::shared_ptr<Event> event)
{
    if(event){
        m_eventsStream->pushEventToProcess(event);
        VeilFS::getScheduler()->addTask(Job(time(NULL) + 1, m_eventsStream, ISchedulable::TASK_PROCESS_EVENT));
    }
}

bool EventCommunicator::runTask(TaskID taskId, const string &arg0, const string &arg1, const string &arg2)
{
    switch(taskId)
    {
    case TASK_GET_EVENT_PRODUCER_CONFIG:
        configureByCluster();
        return true;

    case TASK_IS_WRITE_ENABLED:
        m_writeEnabled = askClusterIfWriteEnabled();
        return true;

    default:
        return false;
    }
}

void EventCommunicator::addStatAfterWritesRule(int bytes){
    boost::shared_ptr<IEventStream> filter(new EventFilter("type", "write_event"));
    boost::shared_ptr<IEventStream> aggregator(new EventAggregator(filter, "filePath", bytes, "bytes"));
    boost::shared_ptr<IEventStream> customAction(new CustomActionStream(aggregator, boost::bind(&EventCommunicator::statFromWriteEvent, this, _1)));

    addEventSubstream(customAction);
}

void EventCommunicator::setFslogic(boost::shared_ptr<FslogicProxy> fslogicProxy)
{
    m_fslogic = fslogicProxy;
}

void EventCommunicator::setMetaCache(boost::shared_ptr<MetaCache> metaCache)
{
    m_metaCache = metaCache;
}

bool EventCommunicator::isWriteEnabled()
{
    return m_writeEnabled;
}

boost::shared_ptr<Event> EventCommunicator::statFromWriteEvent(boost::shared_ptr<Event> event){
    string path = event->getStringProperty("filePath", "");
    if(!path.empty() && m_metaCache && m_fslogic){
        time_t currentTime = time(NULL);
        m_fslogic->updateTimes(path, 0, currentTime, currentTime);

        FileAttr attr;
        m_metaCache->clearAttr(path);
        if(VeilFS::getOptions()->get_enable_attr_cache()){
            // TODO: The whole mechanism we force attributes to be reloaded is inefficient - we just want to cause attributes to be changed on cluster but
            // we also fetch attributes
            m_fslogic->getFileAttr(string(path), attr);
        }
    }

    // we don't want to forward this event - it has already been handled by this function
    return boost::shared_ptr<Event> ();
}

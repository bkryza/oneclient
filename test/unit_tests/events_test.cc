/**
 * @file events_test.cc
 * @author Michal Sitko
 * @copyright (C) 2013 ACK CYFRONET AGH
 * @copyright This software is released under the MIT license cited in 'LICENSE.txt'
 */

#include "testCommon.h"
#include "events.h"
#include "events_mock.h"
#include "boost/shared_ptr.hpp"

using namespace boost;

INIT_AND_RUN_ALL_TESTS(); // TEST RUNNER !

// TEST definitions below

class EventsTest 
    : public ::testing::Test
{
public:

    virtual void SetUp() {
        
    }

    virtual void TearDown() {
        
    }
};

// checks simple stream with single EventFilter
TEST(EventFilter, SimpleFilter) {
	// given
	shared_ptr<Event> mkdirEvent = Event::createMkdirEvent("user1", "file1");
	shared_ptr<Event> writeEvent = Event::createWriteEvent("user1", "file2", 100);
	EventFilter filter("type", "mkdir_event");

	// what
	shared_ptr<Event> resEvent = filter.processEvent(writeEvent);
	ASSERT_EQ(0, resEvent.use_count());

	resEvent = filter.processEvent(mkdirEvent);
	ASSERT_EQ(1, resEvent.use_count());
	ASSERT_EQ(string("user1"), resEvent->getProperty("userId", string("")));
	ASSERT_EQ(string("file1"), resEvent->getProperty("fileId", string("")));
}

// checks simple stream with single EventAggregator
TEST(EventAggregatorTest, SimpleAggregation) {
	// given
	shared_ptr<Event> mkdirEvent = Event::createMkdirEvent("user1", "file1");
	shared_ptr<Event> writeEvent = Event::createWriteEvent("user1", "file1", 100);
	EventAggregator aggregator(5);

	// what
	for(int i=0; i<4; ++i){
		shared_ptr<Event> res = aggregator.processEvent(mkdirEvent);
		ASSERT_EQ(0, res.use_count());
	}

	// then
	shared_ptr<Event> res = aggregator.processEvent(writeEvent);
	ASSERT_EQ(1, res.use_count());

	// with aggregator configured that way there should be just one property
	ASSERT_EQ(1, res->properties.size());
	ASSERT_EQ(5L, res->getProperty<long long>("count", -1L));

	for(int i=0; i<4; ++i){
		shared_ptr<Event> res = aggregator.processEvent(mkdirEvent);
		ASSERT_EQ(0, res.use_count());
	}

	res = aggregator.processEvent(writeEvent);
	ASSERT_EQ(1, res.use_count());
	ASSERT_EQ(1, res->properties.size());
	ASSERT_EQ(5, res->getProperty<long long>("count", -1L));
}

TEST(EventAggregatorTest, AggregationByOneField) {
	// given
	shared_ptr<Event> mkdirEvent = Event::createMkdirEvent("user1", "file1");
	shared_ptr<Event> writeEvent = Event::createWriteEvent("user1", "file1", 100);
	EventAggregator aggregator("type", 5);

	// what
	for(int i=0; i<4; ++i){
		shared_ptr<Event> res = aggregator.processEvent(mkdirEvent);
		ASSERT_EQ(0, res.use_count());	
	}
	shared_ptr<Event> res = aggregator.processEvent(writeEvent);
	ASSERT_EQ(0, res.use_count());

	res = aggregator.processEvent(mkdirEvent);
	ASSERT_EQ(1, res.use_count());
	ASSERT_EQ(2, res->properties.size());
	ASSERT_EQ(5, res->getProperty<long long>("count", -1L));
	ASSERT_EQ("mkdir_event", res->getProperty("type", string("")));
	
	// we are sending just 3 writeEvents because one has already been sent
	for(int i=0; i<3; ++i){
		shared_ptr<Event> res = aggregator.processEvent(writeEvent);
		ASSERT_EQ(0, res.use_count());	
	}

	res = aggregator.processEvent(mkdirEvent);
	ASSERT_EQ(0, res.use_count());

	res = aggregator.processEvent(writeEvent);
	ASSERT_EQ(1, res.use_count());
	ASSERT_EQ(2, res->properties.size());
	ASSERT_EQ(5, res->getProperty<long long>("count", -1L));
	ASSERT_EQ("write_event", res->getProperty("type", string("")));
}

TEST(EventAggregatorTest, FilterAndAggregation) {
	// given
	shared_ptr<Event> file1Event = Event::createMkdirEvent("user1", "file1");
	shared_ptr<Event> file2Event = Event::createMkdirEvent("user1", "file2");
	shared_ptr<Event> writeEvent = Event::createWriteEvent("user1", "file1", 100);
	shared_ptr<Event> writeEvent2 = Event::createWriteEvent("user1", "file2", 100);
	shared_ptr<IEventStream> filter(new EventFilter("type", "mkdir_event"));
	shared_ptr<IEventStream> aggregator(new EventAggregator(filter, "fileId", 5));

	for(int i=0; i<4; ++i){
		shared_ptr<Event> res = aggregator->processEvent(file1Event);
		ASSERT_EQ(0, res.use_count());
	}

	shared_ptr<Event> res = aggregator->processEvent(file2Event);
	ASSERT_EQ(0, res.use_count());

	res = aggregator->processEvent(writeEvent);
	ASSERT_EQ(0, res.use_count());

	res = aggregator->processEvent(file1Event);
	ASSERT_EQ(1, res.use_count());
	ASSERT_EQ(2, res->properties.size());
	ASSERT_EQ(5, res->getProperty<long long>("count", -1L));
	ASSERT_EQ("file1", res->getProperty("fileId", string("")));

	for(int i=0; i<3; ++i){
		shared_ptr<Event> res = aggregator->processEvent(file2Event);
		ASSERT_EQ(0, res.use_count());
	}

	res = aggregator->processEvent(file2Event);
	ASSERT_EQ(1, res.use_count());
	ASSERT_EQ(2, res->properties.size());
	ASSERT_EQ(5, res->getProperty<long long>("count", -1L));
	ASSERT_EQ("file2", res->getProperty("fileId", string("")));

	for(int i=0; i<5; ++i){
		shared_ptr<Event> res = aggregator->processEvent(writeEvent2);
		ASSERT_EQ(0, res.use_count());
	}
}

TEST(EventStreamCombiner, CombineStreams) {
	shared_ptr<Event> mkdirEvent = Event::createMkdirEvent("user1", "file1");
	shared_ptr<Event> writeEvent = Event::createWriteEvent("user1", "file1", 100);
	shared_ptr<IEventStream> mkdirFilter(new EventFilter("type", "mkdir_event"));
	EventStreamCombiner combiner;
	combiner.m_substreams.push_back(mkdirFilter);

	list<shared_ptr<Event> > events = combiner.processEvent(mkdirEvent);
	ASSERT_EQ(1, events.size());

	events = combiner.processEvent(writeEvent);
	ASSERT_EQ(0, events.size());

	shared_ptr<IEventStream> writeFilter(new EventFilter("type", "write_event"));
	combiner.m_substreams.push_back(writeFilter);

	events = combiner.processEvent(writeEvent);
	ASSERT_EQ(1, events.size());

	events = combiner.processEvent(mkdirEvent);
	ASSERT_EQ(1, events.size());
}

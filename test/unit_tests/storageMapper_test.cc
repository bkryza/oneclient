/**
 * @file storageMapper_test.cc
 * @author Rafal Slota
 * @copyright (C) 2013 ACK CYFRONET AGH
 * @copyright This software is released under the MIT license cited in 'LICENSE.txt'
 */

#include "testCommon.h"
#include "storageMapper_proxy.h"
#include "config_mock.h"
#include "fslogicProxy_mock.h"
#include "jobScheduler_mock.h"
#include "veilfs.h"


INIT_AND_RUN_ALL_TESTS(); // TEST RUNNER !

// TEST definitions below

class StorageMapperTest 
    : public ::testing::Test {

protected:
    COMMON_DEFS();
    shared_ptr<MockFslogicProxy> mockFslogic; 
    shared_ptr<ProxyStorageMapper> proxy;    

    virtual void SetUp() {
        COMMON_SETUP();
        mockFslogic.reset(new MockFslogicProxy());
        proxy.reset(new ProxyStorageMapper(mockFslogic));

    }

    virtual void TearDown() {
        COMMON_CLEANUP();
    }

};

TEST_F(StorageMapperTest, AddAndGet) {
    EXPECT_EQ(0, proxy->getStorageMapping().size());
    EXPECT_EQ(0, proxy->getFileMapping().size());

    FileLocation location;
    location.set_validity(10);
    location.set_storage_id(1);
    location.add_storage_helper_args("arg0");
    location.add_storage_helper_args("arg1");

    EXPECT_THROW(proxy->getLocationInfo("/file1"), VeilException);
    EXPECT_CALL(*scheduler, addTask(_)).Times(2);
    proxy->addLocation("/file1", location);
    EXPECT_EQ(1, proxy->getStorageMapping().size());
    EXPECT_EQ(1, proxy->getFileMapping().size());

    EXPECT_CALL(*scheduler, addTask(_)).Times(2);
    proxy->addLocation("/file1", location);
    EXPECT_EQ(1, proxy->getStorageMapping().size());
    EXPECT_EQ(1, proxy->getFileMapping().size());

    EXPECT_THROW(proxy->getLocationInfo("/file0"), VeilException);
    EXPECT_NO_THROW(proxy->getLocationInfo("/file1"));

    location.set_validity(20);
    location.set_storage_id(2);
    location.clear_storage_helper_args();
    location.add_storage_helper_args("arg2");
    location.add_storage_helper_args("arg3");
    EXPECT_CALL(*scheduler, addTask(_)).Times(2);
    proxy->addLocation("/file2", location);
    EXPECT_NO_THROW(proxy->getLocationInfo("/file2"));

    pair<locationInfo, storageInfo> ret1 = proxy->getLocationInfo("/file1");
    pair<locationInfo, storageInfo> ret2 = proxy->getLocationInfo("/file2");
    EXPECT_EQ(1, ret1.first.storageId);
    EXPECT_EQ(2, ret2.first.storageId);

    EXPECT_EQ("arg0", ret1.second.storageHelperArgs[0]);
    EXPECT_EQ("arg3", ret2.second.storageHelperArgs[1]);
}

TEST_F(StorageMapperTest, OpenClose) {
    EXPECT_CALL(*scheduler, addTask(_)).Times(4);
    FileLocation location;
    proxy->addLocation("/file1", location);
    proxy->addLocation("/file2", location);

    pair<locationInfo, storageInfo> ret1 = proxy->getLocationInfo("/file1");
    pair<locationInfo, storageInfo> ret2 = proxy->getLocationInfo("/file2");

    EXPECT_EQ(0, ret1.first.opened);
    EXPECT_EQ(0, ret2.first.opened);

    proxy->openFile("/file3");
    proxy->openFile("/file1");
    proxy->openFile("/file1");

    EXPECT_EQ(2, proxy->getLocationInfo("/file1").first.opened);
    EXPECT_EQ(0, proxy->getLocationInfo("/file2").first.opened);


    proxy->releaseFile("/file3");
    proxy->releaseFile("/file1");
    proxy->openFile("/file2");
    EXPECT_EQ(1, proxy->getLocationInfo("/file1").first.opened);
    EXPECT_EQ(1, proxy->getLocationInfo("/file2").first.opened);

    proxy->releaseFile("/file1");
    proxy->releaseFile("/file1");
    proxy->releaseFile("/file2");

    EXPECT_EQ(0, proxy->getLocationInfo("/file1").first.opened);
    EXPECT_EQ(0, proxy->getLocationInfo("/file2").first.opened);
}

TEST_F(StorageMapperTest, FindAndGet) {
    EXPECT_CALL(*mockFslogic, getFileLocation("/file1", _)).WillOnce(Return(false));
    EXPECT_NE(VOK, proxy->findLocation("/file1"));

    FileLocation location;
    location.set_answer(VEACCES);
    EXPECT_CALL(*mockFslogic, getFileLocation("/file1", _)).WillOnce(DoAll(SetArgPointee<1>(location), Return(true)));
    EXPECT_EQ(VEACCES, proxy->findLocation("/file1"));

    EXPECT_THROW(proxy->getLocationInfo("/file1"), VeilException);
    location.set_answer(VOK);
    location.set_validity(20);
    time_t currentTime = time(NULL);
    EXPECT_CALL(*scheduler, addTask(_)).Times(2);
    EXPECT_CALL(*mockFslogic, getFileLocation("/file1", _)).WillOnce(DoAll(SetArgPointee<1>(location), Return(true)));
    EXPECT_EQ(VOK, proxy->findLocation("/file1"));

    EXPECT_NO_THROW(proxy->getLocationInfo("/file1"));
    EXPECT_THROW(proxy->getLocationInfo("/file2"), VeilException);

    EXPECT_GE(currentTime + 20, proxy->getLocationInfo("/file1").first.validTo);

    EXPECT_CALL(*mockFslogic, getFileLocation("/file2", _)).WillOnce(Return(false));
    EXPECT_THROW(proxy->getLocationInfo("/file2", true), VeilException);

    EXPECT_CALL(*scheduler, addTask(_)).Times(2);
    EXPECT_CALL(*mockFslogic, getFileLocation("/file2", _)).WillOnce(DoAll(SetArgPointee<1>(location), Return(true)));
    EXPECT_NO_THROW(proxy->getLocationInfo("/file2", true));
}
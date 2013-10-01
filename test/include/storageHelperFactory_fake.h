/**
 * @file storageHelperFactory_fake.h
 * @author Rafal Slota
 * @copyright (C) 2013 ACK CYFRONET AGH
 * @copyright This software is released under the MIT license cited in 'LICENSE.txt'
 */

#ifndef STORAGE_HELPER_FACTORY_FAKE_H
#define STORAGE_HELPER_FACTORY_FAKE_H

#include "helpers/storageHelperFactory.h"
#include "genericHelper_mock.h"
#include "testCommon.h"
#include "gmock/gmock.h"

using namespace veil::helpers;

class FakeStorageHelperFactory
    : public StorageHelperFactory {
public:
    shared_ptr<IStorageHelper> presetMock;

    shared_ptr<IStorageHelper> getStorageHelper(string sh_name, vector<string> args) {
        if(presetMock)
            return presetMock;
        else
            return shared_ptr<IStorageHelper>(new MockGenericHelper());    
    }

};

#endif //STORAGE_HELPER_FACTORY_FAKE_H
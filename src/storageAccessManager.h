/**
 * @file storageAccessManager.h
 * @author Krzysztof Trzepla
 * @copyright (C) 2016 ACK CYFRONET AGH
 * @copyright This software is released under the MIT license cited in
 * 'LICENSE.txt'
 */

#ifndef ONECLIENT_STORAGE_ACCESS_MANAGER_H
#define ONECLIENT_STORAGE_ACCESS_MANAGER_H

#include "communication/communicator.h"

#include <boost/filesystem.hpp>

#include <vector>

namespace one {
namespace helpers {
class IStorageHelper;
class StorageHelperFactory;
}
namespace messages {
namespace fuse {
class StorageTestFile;
}
}
namespace client {

/**
 * The StorageAccessManager class is responsible for detecting storages that are
 * directly accessible to the client.
 */
class StorageAccessManager {
public:
    /**
     * Constructor.
     * @param communicator Instance of @c communication::Communicator.
     * @param helperFactory Instance of @c helpers::StorageHelperFactory.
     */
    StorageAccessManager(communication::Communicator &communicator,
        helpers::StorageHelperFactory &helperFactory);

    /**
     * Verifies the test file by reading it from the storage and checking its
     * content with the one sent by the server.
     * @param testFile Instance of @c messages::fuse::StorageTestFile.
     * @return Storage helper object used to access the test file or nullptr if
     * verification fails.
     */
    std::shared_ptr<helpers::IStorageHelper> verifyStorageTestFile(
        const messages::fuse::StorageTestFile &testFile);

    /**
     * Modifies the test file by writing random sequence of characters.
     * @param helper Storage helper object used to access the test file.
     * @param testFile Instance of @c messages::fuse::StorageTestFile.
     * @return Modified content of the test file.
     */
    std::string modifyStorageTestFile(
        std::shared_ptr<helpers::IStorageHelper> helper,
        const messages::fuse::StorageTestFile &testFile);

private:
    bool verifyStorageTestFile(std::shared_ptr<helpers::IStorageHelper> helper,
        const messages::fuse::StorageTestFile &testFile);

    communication::Communicator &m_communicator;
    helpers::StorageHelperFactory &m_helperFactory;
    std::vector<boost::filesystem::path> m_mountPoints;
};

} // namespace client
} // namespace one

#endif // ONECLIENT_STORAGE_ACCESS_MANAGER_H

/**
 * @file fsLogic.h
 * @author Rafal Slota
 * @copyright (C) 2015 ACK CYFRONET AGH
 * @copyright This software is released under the MIT license cited in
 * 'LICENSE.txt'
 */

#ifndef ONECLIENT_FS_LOGIC_H
#define ONECLIENT_FS_LOGIC_H

#include "cache/cacheExpirationHelper.h"
#include "cache/fileContextCache.h"
#include "cache/forceProxyIOCache.h"
#include "cache/helpersCache.h"
#include "cache/metadataCache.h"
#include "events/eventManager.h"
#include "fsSubscriptions.h"
#include "messages/fuse/checksum.h"
#include "messages/fuse/fileAttr.h"
#include "messages/fuse/fileLocation.h"
#include "messages/fuse/helperParams.h"

#include <asio/buffer.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>
#include <fuse.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_unordered_set.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <tuple>
#include <vector>

namespace one {

namespace messages {
class Configuration;
namespace fuse {
class GetFileAttr;
}
}

namespace client {

class Context;

namespace events {
class EventManager;
}

/**
 * The FsLogic main class.
 * This class contains FUSE all callbacks, so it basically is an heart of the
 * filesystem. Technically FsLogic is an singleton created on program start and
 * registered in FUSE daemon.
 */
class FsLogic {
public:
    /**
     * Constructor.
     * @param context Shared pointer to application context instance.
     */
    FsLogic(std::shared_ptr<Context> context,
        std::shared_ptr<messages::Configuration> configuration);

    /**
    * Destructor.
    * Cancels scheduling operations.
    */
    ~FsLogic();

    /**
     * FUSE @c access callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int access(boost::filesystem::path path, const int mode);

    /**
     * FUSE @c getattr callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int getattr(boost::filesystem::path path, struct stat *const statbuf);

    /**
     * FUSE @c readlink callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int readlink(boost::filesystem::path path, asio::mutable_buffer buf);

    /**
     * FUSE @c mknod callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int mknod(boost::filesystem::path path, const mode_t mode, const dev_t dev);

    /**
     * FUSE @c mkdir callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int mkdir(boost::filesystem::path path, const mode_t mode);

    /**
     * FUSE @c unlink callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int unlink(boost::filesystem::path path);

    /**
     * FUSE @c rmdir callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int rmdir(boost::filesystem::path path);

    /**
     * FUSE @c symlink callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int symlink(
        boost::filesystem::path target, boost::filesystem::path linkPath);

    /**
     * FUSE @c rename callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int rename(
        boost::filesystem::path oldPath, boost::filesystem::path newPath);

    /**
     * FUSE @c chmod callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int chmod(boost::filesystem::path path, const mode_t mode);

    /**
     * FUSE @c chown callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int chown(boost::filesystem::path path, const uid_t uid, const gid_t gid);

    /**
     * FUSE @c truncate callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int truncate(boost::filesystem::path path, const off_t newSize);

    /**
     * FUSE @c utime callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int utime(boost::filesystem::path path, struct utimbuf *const ubuf);

    /**
     * FUSE @c open callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int open(
        boost::filesystem::path path, struct fuse_file_info *const fileInfo);

    /**
     * FUSE @c read callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int read(boost::filesystem::path path, asio::mutable_buffer buf,
        const off_t offset, struct fuse_file_info *const fileInfo);

    /**
     * FUSE @c write callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int write(boost::filesystem::path path, asio::const_buffer buf,
        const off_t offset, struct fuse_file_info *const fileInfo);

    /**
     * FUSE @c statfs callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int statfs(boost::filesystem::path path, struct statvfs *const statInfo);

    /**
     * FUSE @c flush callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int flush(
        boost::filesystem::path path, struct fuse_file_info *const fileInfo);

    /**
     * FUSE @c release callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int release(
        boost::filesystem::path path, struct fuse_file_info *const fileInfo);

    /**
     * FUSE @c fsync callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int fsync(boost::filesystem::path path, const int datasync,
        struct fuse_file_info *const fileInfo);

    /**
     * FUSE @c opendir callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int opendir(
        boost::filesystem::path path, struct fuse_file_info *const fileInfo);

    /**
     * FUSE @c readdir callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int readdir(boost::filesystem::path path, void *const buf,
        const fuse_fill_dir_t filler, const off_t offset,
        struct fuse_file_info *fileInfo);

    /**
     * FUSE @c releasedir callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int releasedir(
        boost::filesystem::path path, struct fuse_file_info *const fileInfo);

    /**
     * FUSE @c fsyncdir callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int fsyncdir(boost::filesystem::path path, const int datasync,
        struct fuse_file_info *const fileInfo);

    /**
     * FUSE @c create callback.
     * @see http://fuse.sourceforge.net/doxygen/structfuse__operations.html
     */
    int create(boost::filesystem::path path, const mode_t mode,
        struct fuse_file_info *const fileInfo);

protected:
    virtual HelpersCache::HelperPtr getHelper(
        const std::string &fileUuid, const std::string &storageId);

private:
    void scheduleCacheExpirationTick();
    void removeFile(boost::filesystem::path path);
    const std::string createFile(
        boost::filesystem::path path, mode_t, const one::helpers::FlagsSet);
    void openFile(
        const std::string &fileUuid, struct fuse_file_info *const fileInfo);
    one::messages::fuse::Checksum waitForBlockSynchronization(
        const std::string &uuid,
        const boost::icl::discrete_interval<off_t> &range,
        const one::helpers::FlagsSet flags);
    one::messages::fuse::Checksum syncAndFetchChecksum(const std::string &uuid,
        const boost::icl::discrete_interval<off_t> &range);
    std::tuple<messages::fuse::FileBlock, asio::const_buffer> findWriteLocation(
        const messages::fuse::FileLocation &fileLocation, const off_t offset,
        const asio::const_buffer &buf);
    events::FileAttrEventStream::Handler fileAttrHandler();
    events::FileLocationEventStream::Handler fileLocationHandler();
    events::PermissionChangedEventStream::Handler permissionChangedHandler();
    events::FileRemovalEventStream::Handler fileRemovalHandler();
    events::QuotaExeededEventStream::Handler quotaExeededHandler();
    events::FileRenamedEventStream::Handler fileRenamedHandler();
    bool dataCorrupted(const std::string &uuid, asio::const_buffer buf,
        const messages::fuse::Checksum &serverChecksum,
        const boost::icl::discrete_interval<off_t> &availableRange,
        const boost::icl::discrete_interval<off_t> &wantedRange);
    std::string computeHash(asio::const_buffer buf);
    void disableSpaces(const std::vector<std::string> &spaces);

    const uid_t m_uid;
    const gid_t m_gid;
    const unsigned long m_fsid;

    std::shared_ptr<Context> m_context;
    events::EventManager m_eventManager;
    FileContextCache m_fileContextCache;
    HelpersCache m_helpersCache;
    MetadataCache m_metadataCache;
    FsSubscriptions m_fsSubscriptions;
    ForceProxyIOCache m_forceProxyIOCache;
    CacheExpirationHelper<std::string> m_locExpirationHelper;
    CacheExpirationHelper<std::string> m_attrExpirationHelper;

    std::mutex m_cancelCacheExpirationTickMutex;
    std::function<void()> m_cancelCacheExpirationTick;
    std::shared_timed_mutex m_disabledSpacesMutex;
    tbb::concurrent_unordered_set<std::string> m_disabledSpaces;
};

struct FsLogicWrapper {
    std::unique_ptr<FsLogic> logic;
};

} // namespace client
} // namespace one

#endif // ONECLIENT_FS_LOGIC_H

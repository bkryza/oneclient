/**
 * @file forceProxyIOCache.cc
 * @author Tomasz Lichon
 * @copyright (C) 2016 ACK CYFRONET AGH
 * @copyright This software is released under the MIT license cited in
 * 'LICENSE.txt'
 */

#include "forceProxyIOCache.h"

namespace one {
namespace client {

ForceProxyIOCache::ForceProxyIOCache(FsSubscriptions &fsSubscriptions)
    : m_fsSubscriptions{fsSubscriptions}
{
}

bool ForceProxyIOCache::contains(const std::string &fileUuid)
{
#if !defined(USE_BOOST_SHARED_MUTEX)
    std::shared_lock<std::shared_timed_mutex> lock{m_cacheMutex};
#else
    std::shared_lock<boost::shared_mutex> lock{m_cacheMutex};
#endif

    return m_cache.find(fileUuid) != m_cache.end();
}

void ForceProxyIOCache::insert(const std::string &fileUuid)
{
    {
#if !defined(USE_BOOST_SHARED_MUTEX)
        std::shared_lock<std::shared_timed_mutex> lock{m_cacheMutex};
#else
        std::shared_lock<boost::shared_mutex> lock{m_cacheMutex};
#endif
        m_cache.insert(fileUuid);
    }
    m_fsSubscriptions.addPermissionChangedSubscription(fileUuid);
}

void ForceProxyIOCache::erase(const std::string &fileUuid)
{
    {
#if !defined(USE_BOOST_SHARED_MUTEX)
        std::shared_lock<std::shared_timed_mutex> lock{m_cacheMutex};
#else
        std::shared_lock<boost::shared_mutex> lock{m_cacheMutex};
#endif       
        m_cache.unsafe_erase(fileUuid);
    }
    m_fsSubscriptions.removePermissionChangedSubscription(fileUuid);
}

} // namespace one
} // namespace client

/**
 * @file context.h
 * @author Konrad Zemek
 * @copyright (C) 2014 ACK CYFRONET AGH
 * @copyright This software is released under the MIT license cited in 'LICENSE.txt'
 */

#ifndef VEILCLIENT_CONTEXT_H
#define VEILCLIENT_CONTEXT_H


#include "ISchedulable.h"

#include <boost/thread/shared_mutex.hpp>

#include <memory>
#include <mutex>
#include <list>

namespace veil
{

class SimpleConnectionPool;

namespace client
{

class Options;
class Config;
class JobScheduler;
class PushListener;
class StorageMapper;

class Context
{
public:
    std::shared_ptr<Options> getOptions() const;
    void setOptions(std::shared_ptr<Options> options);

    std::shared_ptr<Config> getConfig() const;
    void setConfig(std::shared_ptr<Config> config);

    std::shared_ptr<JobScheduler> getScheduler(const ISchedulable::TaskID taskId = ISchedulable::TaskID::TASK_LAST_ID);
    void addScheduler(std::shared_ptr<JobScheduler> scheduler);

    std::shared_ptr<SimpleConnectionPool> getConnectionPool() const;
    void setConnectionPool(std::shared_ptr<SimpleConnectionPool> connectionPool);

    std::shared_ptr<PushListener> getPushListener() const;
    void setPushListener(std::shared_ptr<PushListener> pushListener);

    std::shared_ptr<StorageMapper> getStorageMapper() const;
    void setStorageMapper(std::shared_ptr<StorageMapper>);

private:
    std::shared_ptr<Options> m_options;
    std::shared_ptr<Config> m_config;
    std::list<std::shared_ptr<JobScheduler>> m_jobSchedulers;
    std::shared_ptr<SimpleConnectionPool> m_connectionPool;
    std::shared_ptr<PushListener> m_pushListener;
    std::shared_ptr<StorageMapper> m_storageMapper;

    mutable boost::shared_mutex m_optionsMutex;
    mutable boost::shared_mutex m_configMutex;
    std::mutex m_jobSchedulersMutex;
    mutable boost::shared_mutex m_connectionPoolMutex;
    mutable boost::shared_mutex m_pushListenerMutex;
    mutable boost::shared_mutex m_storageMapperMutex;
};

} // namespace client
} // namespace veil

#endif // VEILCLIENT_CONTEXT_H

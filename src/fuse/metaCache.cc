/**
 * @file metaCache.cc
 * @author Rafal Slota
 * @copyright (C) 2013 ACK CYFRONET AGH
 * @copyright This software is released under the MIT license cited in 'LICENSE.txt'
 */

#include "metaCache.hh"
#include "jobScheduler.hh"
#include "veilfs.hh"
#include "config.hh"

#include "glog/logging.h"

#include <memory.h>

MetaCache::MetaCache()
{
}

MetaCache::~MetaCache()
{
}

void MetaCache::addAttr(string path, struct stat attr)
{
    if(Config::isSet(ENABLE_ATTR_CACHE_OPT) && !Config::getValue<bool>(ENABLE_ATTR_CACHE_OPT))
        return;

    AutoLock lock(m_statMapLock, WRITE_LOCK);
    bool wasBefore = getAttr(path, NULL);
    m_statMap[path] = make_pair(time(NULL), attr);

    if(!wasBefore)
    {
        int expiration_time = Config::isSet(ATTR_CACHE_EXPIRATION_TIME_OPT) ? Config::getValue<int>(ATTR_CACHE_EXPIRATION_TIME_OPT) : ATTR_DEFAULT_EXPIRATION_TIME;
        // because of random part, only small parts of cache will be updated at the same moment
        int when = time(NULL) + expiration_time / 2 + rand() % expiration_time;
        VeilFS::getScheduler()->addTask(Job(when, this, TASK_CLEAR_FILE_ATTR, path));
    }
}

bool MetaCache::getAttr(string path, struct stat* attr)
{
    AutoLock lock(m_statMapLock, READ_LOCK);
    map<string, pair<time_t, struct stat> >::iterator it = m_statMap.find(path);
    if(it == m_statMap.end())
        return false;

    if(attr != NULL) // NULL pointer is allowed to be used as parameter
        memcpy(attr, &(*it).second.second, sizeof(struct stat));

    return true;
}

void MetaCache::clearAttrs()
{
    AutoLock lock(m_statMapLock, WRITE_LOCK);
    m_statMap.clear();
}

void MetaCache::clearAttr(string path)
{
    AutoLock lock(m_statMapLock, WRITE_LOCK);
    LOG(INFO) << "delete attrs from cache for file: " << path;
    map<string, pair<time_t, struct stat> >::iterator it = m_statMap.find(path);
    if(it != m_statMap.end())
        m_statMap.erase(it);
}

bool MetaCache::runTask(TaskID taskId, string arg0, string arg1, string arg3)
{
    switch(taskId)
    {
    case TASK_CLEAR_FILE_ATTR:
        clearAttr(arg0);
        return true;
    default:
        return false;
    }
}

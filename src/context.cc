/**
 * @file context.cc
 * @author Konrad Zemek
 * @copyright (C) 2014 ACK CYFRONET AGH
 * @copyright This software is released under the MIT license cited in
 * 'LICENSE.txt'
 */

#include "context.h"

#include <algorithm>
#include <atomic>

namespace one {
namespace client {

std::shared_ptr<Options> Context::options() const
{
#if !defined(USE_BOOST_SHARED_MUTEX)
    std::shared_lock<std::shared_timed_mutex> lock{m_optionsMutex};
#else
    std::shared_lock<boost::shared_mutex> lock{m_optionsMutex};
#endif

    return m_options;
}

void Context::setOptions(std::shared_ptr<Options> opt)
{
#if !defined(USE_BOOST_SHARED_MUTEX)
    std::shared_lock<std::shared_timed_mutex> lock{m_optionsMutex};
#else
    std::shared_lock<boost::shared_mutex> lock{m_optionsMutex};
#endif     

    m_options = std::move(opt);
}

std::shared_ptr<Scheduler> Context::scheduler() const
{
#if !defined(USE_BOOST_SHARED_MUTEX)
    std::shared_lock<std::shared_timed_mutex> lock{m_schedulerMutex};
#else
    std::shared_lock<boost::shared_mutex> lock{m_schedulerMutex};
#endif
    
    return m_scheduler;
}

void Context::setScheduler(std::shared_ptr<Scheduler> sched)
{
#if !defined(USE_BOOST_SHARED_MUTEX)
    std::shared_lock<std::shared_timed_mutex> lock{m_schedulerMutex};
#else
    std::shared_lock<boost::shared_mutex> lock{m_schedulerMutex};
#endif

    m_scheduler = std::move(sched);
}

std::shared_ptr<communication::Communicator> Context::communicator() const
{
#if !defined(USE_BOOST_SHARED_MUTEX)
    std::shared_lock<std::shared_timed_mutex> lock{m_communicatorMutex};
#else
    std::shared_lock<boost::shared_mutex> lock{m_communicatorMutex};
#endif

    return m_communicator;
}

void Context::setCommunicator(std::shared_ptr<communication::Communicator> comm)
{
#if !defined(USE_BOOST_SHARED_MUTEX)
    std::shared_lock<std::shared_timed_mutex> lock{m_communicatorMutex};
#else
    std::shared_lock<boost::shared_mutex> lock{m_communicatorMutex};
#endif

    m_communicator = std::move(comm);
}

} // namespace client
} // namespace one

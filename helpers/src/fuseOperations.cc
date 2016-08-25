/**
 * @file fuseOperations.cc
 * @author Krzysztof Trzepla
 * @copyright (C) 2016 ACK CYFRONET AGH
 * @copyright This software is released under the MIT license cited in
 * 'LICENSE.txt'
 */

#include "fuseOperations.h"

namespace {
#if !defined(__APPLE__)
thread_local 
#endif
bool fuseSessionActive = false;
} // namespace

namespace one {
namespace helpers {

void activateFuseSession() { fuseSessionActive = true; }

bool fuseInterrupted() { return fuseSessionActive && fuse_interrupted(); }

} // namespace helpers
} // namespace one
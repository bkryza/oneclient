/**
 * @file pong.cc
 * @author Krzysztof Trzepla
 * @copyright (C) 2015 ACK CYFRONET AGH
 * @copyright This software is released under the MIT license cited in
 * 'LICENSE.txt'
 */

#include "messages/pong.h"

#include "messages.pb.h"

namespace one {
namespace messages {

Pong::Pong(std::unique_ptr<ProtocolServerMessage> serverMessage) {}

std::string Pong::toString() const { return "type: 'Pong'"; }

} // namespace messages
} // namespace one

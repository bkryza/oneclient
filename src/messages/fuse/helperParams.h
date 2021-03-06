/**
 * @file helperParams.h
 * @author Konrad Zemek
 * @copyright (C) 2015 ACK CYFRONET AGH
 * @copyright This software is released under the MIT license cited in
 * 'LICENSE.txt'
 */

#ifndef ONECLIENT_MESSAGES_FUSE_HELPERS_PARAMS_H
#define ONECLIENT_MESSAGES_FUSE_HELPERS_PARAMS_H

#include "fuseResponse.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace one {
namespace clproto {
class HelperParams;
}
namespace messages {
namespace fuse {

/**
 * The HelperParams class represents server-sent parameters for a helpers
 * instance.
 */
class HelperParams : public FuseResponse {
public:
    using ProtocolMessage = one::clproto::HelperParams;

    HelperParams() = default;

    /**
     * Constructor.
     * @param serverMessage Protocol Buffers message representing
     * @c ServerMessage.
     */
    HelperParams(std::unique_ptr<ProtocolServerMessage> serverMessage);

    /**
     * Constructor.
     * @param message Protocol Buffers message representing @c HelperParams
     * counterpart.
     */
    HelperParams(ProtocolMessage &message);

    /**
     * Constructor.
     * @param name Storage helper name.
     * @param args Storage helper arguments.
     */
    HelperParams(
        std::string name, std::unordered_map<std::string, std::string> args);

    /**
     * @return Helper's name.
     */
    const std::string &name() const { return m_name; }

    /**
     * @return Helper's arguments.
     */
    const std::unordered_map<std::string, std::string> &args() const
    {
        return m_args;
    }

    std::string toString() const override;

private:
    void deserialize(ProtocolMessage &message);

    std::string m_name;
    std::unordered_map<std::string, std::string> m_args;
};

} // namespace fuse
} // namespace messages
} // namespace one

#endif // ONECLIENT_MESSAGES_FUSE_HELPERS_PARAMS_H

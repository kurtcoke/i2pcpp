/**
 * @file AcknowledgementManager.h
 * @brief Defines i2cpp::SSU::AcknowledgmentManager.
 */
#ifndef SSUACKNOWLEDGEMENTMANAGER_H
#define SSUACKNOWLEDGEMENTMANAGER_H

#include <i2pcpp/Log.h>

#include <boost/asio.hpp>

namespace i2pcpp {
    namespace SSU {
        class Context;

        /**
         * Manages acknowledgment (ACK) of receieved data.
         */
        class AcknowledgementManager {
            public:

                /**
                 * Constructs given a reference to an i2pcpp::SSU::Context object.
                 */
                AcknowledgementManager(Context &c);

                AcknowledgementManager(const AcknowledgementManager &) = delete;
                AcknowledgementManager& operator=(AcknowledgementManager &) = delete;

            private:
                /**
                 * For each peer, sends a data packet to acknowedge the fragments
                 *  (both partial and complete) that have been received from it.
                 * This is invoked exactly once every second.
                 */
                void flushAckCallback(const boost::system::error_code& e);

                /// Reference to the i2pcpp::SSU::Context object.
                Context& m_context;

                /// Timer to invoke the ACK callback.
                boost::asio::deadline_timer m_timer;

                i2p_logger_mt m_log;
        };
    }
}

#endif

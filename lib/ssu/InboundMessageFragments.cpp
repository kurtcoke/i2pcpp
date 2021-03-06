/**
 * @file InboundMessageFragments.cpp
 * @brief Implements InboundMessageFragments.h.
 */
#include "InboundMessageFragments.h"

#include "InboundMessageState.h"
#include "Context.h"

#include <i2pcpp/util/make_unique.h>

#include <botan/pipe.h>
#include <botan/filters.h>

#include <string>
#include <bitset>
#include <iomanip>

namespace i2pcpp {
    namespace SSU {
        InboundMessageFragments::InboundMessageFragments(Context &c) :
            m_context(c),
            m_log(I2P_LOG_CHANNEL("IMF")) {}

        void InboundMessageFragments::receiveData(RouterHash const &rh, ByteArrayConstItr &begin, ByteArrayConstItr end)
        {
            I2P_LOG_SCOPED_TAG(m_log, "RouterHash", rh);

            if(std::distance(begin, end) < 1) throw std::runtime_error("malformed SSU data message: 0 length");
            std::bitset<8> flag = *(begin++);

            if(flag[7]) {
                if(std::distance(begin, end) < 1) throw std::runtime_error("malformed SSU data message: ACK bit set; no ACKs");
                unsigned char numAcks = *(begin++);
                if(std::distance(begin, end) < (numAcks * 4)) throw std::runtime_error("malformed SSU data message: length < numAcks");

                while(numAcks--) {
                    uint32_t msgId = parseUint32(begin);

                    std::lock_guard<std::mutex> lock(m_context.omf.m_mutex);
                    m_context.omf.delState(msgId);
                }
            }

            if(flag[6]) {

                unsigned char numFields = *(begin++);
                while(numFields--) {
                    uint32_t msgId = parseUint32(begin);

                    // Read ACK bitfield (1 byte)
                    std::lock_guard<std::mutex> lock(m_context.omf.m_mutex);
                    auto itr = m_context.omf.m_states.find(msgId);
                    uint8_t byteNum = 0;
                    do {
                        uint8_t byte = *begin;
                        for(int i = 6, j = 0; i >= 0; i--, j++) {
                            // If the bit is 1, the fragment has been received
                            if(byte & (1 << i)) {
                                if(itr != m_context.omf.m_states.end())
                                    itr->second.markFragmentAckd((byteNum * 7) + j);
                            }
                        }

                        ++byteNum;
                    // If the low bit is 1, another bitfield follows
                    } while(*(begin++) & (1 << 7));

                    if(itr != m_context.omf.m_states.end() && itr->second.allFragmentsAckd())
                        m_context.omf.delState(msgId);
                }
            }

            if(std::distance(begin, end) < 1) throw std::runtime_error("malformed SSU data message: no body");
            unsigned char numFragments = *(begin++);
            I2P_LOG(m_log, debug) << "number of fragments: " << std::to_string(numFragments);

            for(int i = 0; i < numFragments; i++) {
                if(std::distance(begin, end) < 7) throw std::runtime_error("malformed SSU data message: length of body < 7");
                uint32_t msgId = parseUint32(begin);
                I2P_LOG(m_log, debug) << "fragment[" << i << "] message id: " << std::hex << msgId << std::dec;

                uint32_t fragInfo = (begin[0] << 16) | (begin[1] << 8) | (begin[2]);
                begin += 3;

                uint16_t fragNum = fragInfo >> 17;
                I2P_LOG(m_log, debug) << "fragment[" << i << "] fragment #: " << fragNum;

                bool isLast = (fragInfo & 0x010000);
                I2P_LOG(m_log, debug) << "fragment[" << i << "] isLast: " << isLast;

                uint16_t fragSize = fragInfo & ((1 << 14) - 1);
                I2P_LOG(m_log, debug) << "fragment[" << i << "] size: " << fragSize;

                if(std::distance(begin, end) < fragSize) throw std::runtime_error("malformed SSU data message: length < fragSize");
                ByteArray fragData(begin, begin + fragSize);
                I2P_LOG(m_log, debug) << "fragment[" << i << "] data: " << fragData;

                std::lock_guard<std::mutex> lock(m_mutex);
                auto itr = m_states.get<0>().find(msgId);
                if(itr == m_states.get<0>().end()) {
                    InboundMessageState ims(rh, msgId);
                    ims.addFragment(fragNum, fragData, isLast);

                    checkAndPost(msgId, ims);
                    addState(msgId, rh, std::move(ims));
                } else {
                    m_states.get<0>().modify(itr, AddFragment(fragNum, fragData, isLast));

                    checkAndPost(msgId, itr->state);
                }
            }
        }

        void InboundMessageFragments::addState(const uint32_t msgId, const RouterHash &rh, InboundMessageState ims)
        {
            ContainerEntry sc(std::move(ims));
            sc.msgId = msgId;
            sc.hash = rh;

            auto timer = std::make_unique<boost::asio::deadline_timer>(m_context.ios, boost::posix_time::time_duration(0, 0, 10));
            timer->async_wait(boost::bind(&InboundMessageFragments::timerCallback, this, boost::asio::placeholders::error, msgId));
            sc.timer = std::move(timer);

            m_states.insert(std::move(sc));
        }

        void InboundMessageFragments::delState(const uint32_t msgId)
        {
            m_states.get<0>().erase(msgId);
        }

        void InboundMessageFragments::timerCallback(const boost::system::error_code& e, const uint32_t msgId)
        {
            if(!e) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_states.get<0>().erase(msgId);
            }
        }

        inline void InboundMessageFragments::checkAndPost(const uint32_t msgId, InboundMessageState const &ims)
        {
            if(ims.allFragmentsReceived()) {
                const ByteArray& data = ims.assemble();
                if(data.size())
                    m_context.ios.post(boost::bind(boost::ref(m_context.receivedSignal), ims.getRouterHash(), msgId, data));
            }
        }

        InboundMessageFragments::ContainerEntry::ContainerEntry(InboundMessageState ims) :
            state(std::move(ims)) {}

        InboundMessageFragments::AddFragment::AddFragment(const uint8_t fragNum, ByteArray const &data, bool isLast) :
            m_fragNum(fragNum),
            m_data(data),
            m_isLast(isLast) {}

        void InboundMessageFragments::AddFragment::operator()(ContainerEntry &ce)
        {
            ce.state.addFragment(m_fragNum, m_data, m_isLast);
        }
    }
}

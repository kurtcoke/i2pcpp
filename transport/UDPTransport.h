#ifndef UDPTRANSPORT_H
#define UDPTRANSPORT_H

#include <thread>

#include <boost/asio.hpp>

#include "../datatypes/RouterIdentity.h"

#include "../Log.h"

#include "Transport.h"

#include "ssu/Packet.h"
#include "ssu/PacketHandler.h"
#include "ssu/PeerStateList.h"
#include "ssu/EstablishmentManager.h"
#include "ssu/AcknowledgementScheduler.h"
#include "ssu/OutboundMessageFragments.h"

namespace i2pcpp {
	class UDPTransport : public Transport {
		friend class SSU::PacketHandler;
		friend class SSU::EstablishmentManager;
		friend class SSU::InboundMessageFragments;
		friend class SSU::AcknowledgementScheduler;
		friend class SSU::OutboundMessageFragments;

		public:
			UDPTransport(Botan::DSA_PrivateKey const &privKey, RouterIdentity const &ri);
			~UDPTransport();

			void start(Endpoint const &ep);
			void connect(RouterInfo const &ri);
			void send(RouterHash const &rh, uint32_t msgId, ByteArray const &data);
			void disconnect(RouterHash const &rh);
			uint32_t numPeers() const;
			bool isConnected(RouterHash const &rh) const;
			void shutdown();

		private:
			void sendPacket(SSU::PacketPtr const &p);
			void dataReceived(const boost::system::error_code& e, size_t n);
			void dataSent(const boost::system::error_code& e, size_t n, boost::asio::ip::udp::endpoint ep);

			template<typename CompletionHandler>
			void post(CompletionHandler ch) { m_ios.post(ch); }

			SSU::EstablishmentManager& getEstablisher();

			boost::asio::io_service m_ios;
			boost::asio::ip::udp::socket m_socket;
			boost::asio::ip::udp::endpoint m_senderEndpoint;
			std::array<unsigned char, 2048> m_receiveBuf;

			std::thread m_serviceThread;

			SSU::PeerStateList m_peers;

			SSU::PacketHandler m_packetHandler;
			SSU::EstablishmentManager m_establishmentManager;
			SSU::AcknowledgementScheduler m_ackScheduler;
			SSU::OutboundMessageFragments m_omf;

			i2p_logger_mt m_log;
	};
}

#endif

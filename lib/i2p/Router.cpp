/**
 * @file Router.cpp
 * @brief Implements Router.h
 */
#include "../../include/i2pcpp/Router.h"

#include "Log.h"
#include "RouterContext.h"

#include "transport/UDPTransport.h"

#include <i2pcpp/util/make_unique.h>
#include <i2pcpp/datatypes/RouterInfo.h>

#include <botan/botan.h>
#include <boost/asio.hpp>

#include <thread>

namespace i2pcpp {
    struct Router::RouterImpl {
        RouterImpl(std::string const &dbFile) :
            work(ios),
            ctx(dbFile, ios),
            log(boost::log::keywords::channel = "R") {}

        boost::asio::io_service ios;
        boost::asio::io_service::work work;
        std::thread serviceThread;

        RouterContext ctx;

        i2p_logger_mt log; ///< Logging object
    };

    Router::Router(std::string const &dbFile)
    {
        m_impl = std::make_unique<RouterImpl>(dbFile);
    }

    Router::~Router()
    {
        if(m_impl->serviceThread.joinable()) m_impl->serviceThread.join();
    }

    void Router::initialize()
    {
        Botan::LibraryInitializer init("thread_safe=true");
    }

    void Router::start()
    {
        I2P_LOG(m_impl->log, info) << "local router hash: " << m_impl->ctx.getIdentity()->getHash();

        m_impl->serviceThread = std::thread([&](){
            while(1) {
                try {
                    m_impl->ios.run();
                    break;
                } catch(std::exception &e) {
                    // TODO Backtrace
                    I2P_LOG(m_impl->log, error) << "exception in service thread: " << e.what();
                }
            }
        });

        TransportPtr t = TransportPtr(new UDPTransport(*m_impl->ctx.getSigningKey(), *m_impl->ctx.getIdentity()));
        t->registerReceivedHandler(boost::bind(&InboundMessageDispatcher::messageReceived, boost::ref(m_impl->ctx.getInMsgDisp()), _1, _2, _3));
        t->registerEstablishedHandler(boost::bind(&InboundMessageDispatcher::connectionEstablished, boost::ref(m_impl->ctx.getInMsgDisp()), _1, _2));
        t->registerFailureSignal(boost::bind(&InboundMessageDispatcher::connectionFailure, boost::ref(m_impl->ctx.getInMsgDisp()), _1));
        t->registerDisconnectedSignal(boost::bind(&PeerManager::disconnected, boost::ref(m_impl->ctx.getPeerManager()), _1));
        m_impl->ctx.getOutMsgDisp().registerTransport(t);

        m_impl->ctx.getSignals().registerPeerConnected(boost::bind(&PeerManager::connected, boost::ref(m_impl->ctx.getPeerManager()), _1));
        m_impl->ctx.getSignals().registerPeerConnected(boost::bind(&OutboundMessageDispatcher::connected, boost::ref(m_impl->ctx.getOutMsgDisp()), _1));
        m_impl->ctx.getSignals().registerPeerConnected(boost::bind(&DHT::SearchManager::connected, boost::ref(m_impl->ctx.getSearchManager()), _1));

        m_impl->ctx.getSignals().registerConnectionFailure(boost::bind(&DHT::SearchManager::connectionFailure, boost::ref(m_impl->ctx.getSearchManager()), _1));
        m_impl->ctx.getSignals().registerConnectionFailure(boost::bind(&PeerManager::failure, boost::ref(m_impl->ctx.getPeerManager()), _1));

        m_impl->ctx.getSignals().registerSearchReply(boost::bind(&DHT::SearchManager::searchReply, boost::ref(m_impl->ctx.getSearchManager()), _1, _2, _3));
        m_impl->ctx.getSignals().registerDatabaseStore(boost::bind(&DHT::SearchManager::databaseStore, boost::ref(m_impl->ctx.getSearchManager()), _1, _2, _3));

        m_impl->ctx.getSignals().registerTunnelRecordsReceived(boost::bind(&TunnelManager::receiveRecords, boost::ref(m_impl->ctx.getTunnelManager()), _1, _2));
        m_impl->ctx.getSignals().registerTunnelGatewayData(boost::bind(&TunnelManager::receiveGatewayData, boost::ref(m_impl->ctx.getTunnelManager()), _1, _2, _3));
        m_impl->ctx.getSignals().registerTunnelData(boost::bind(&TunnelManager::receiveData, boost::ref(m_impl->ctx.getTunnelManager()), _1, _2, _3));

        m_impl->ctx.getSearchManager().registerSuccess(boost::bind(&OutboundMessageDispatcher::dhtSuccess, boost::ref(m_impl->ctx.getOutMsgDisp()), _1, _2));
        m_impl->ctx.getSearchManager().registerFailure(boost::bind(&OutboundMessageDispatcher::dhtFailure, boost::ref(m_impl->ctx.getOutMsgDisp()), _1));

        std::shared_ptr<UDPTransport> u = std::static_pointer_cast<UDPTransport>(t);
        u->start(Endpoint(m_impl->ctx.getDatabase().getConfigValue("ssu_bind_ip"), std::stoi(m_impl->ctx.getDatabase().getConfigValue("ssu_bind_port"))));

        m_impl->ctx.getPeerManager().begin();
        //m_impl->ctx.getTunnelManager().begin();
    }

    void Router::stop()
    {
        m_impl->ios.stop();
    }

    ByteArray Router::getRouterInfo()
    {
        // TODO Get this out of here
        Mapping am;
        am.setValue("caps", "BC");
        am.setValue("host", m_impl->ctx.getDatabase().getConfigValue("ssu_external_ip"));
        am.setValue("key", Base64::encode(m_impl->ctx.getIdentity()->getHash()));
        am.setValue("port", m_impl->ctx.getDatabase().getConfigValue("ssu_external_port"));
        RouterAddress a(5, Date(0), "SSU", am);

        Mapping rm;
        rm.setValue("coreVersion", "0.9.9");
        rm.setValue("netId", "2");
        rm.setValue("router.version", "0.9.9");
        rm.setValue("stat_uptime", "90m");
        rm.setValue("caps", "OR");
        RouterInfo myInfo(*m_impl->ctx.getIdentity(), Date(), rm);
        myInfo.addAddress(a);
        myInfo.sign(m_impl->ctx.getSigningKey());

        return myInfo.serialize();
    }

    void Router::importRouter(RouterInfo const &router)
    {
        m_impl->ctx.getDatabase().setRouterInfo(router);
    }

    void Router::importRouter(std::vector<RouterInfo> const &routers)
    {
        m_impl->ctx.getDatabase().setRouterInfo(routers);
    }

    void Router::deleteAllRouters()
    {
        m_impl->ctx.getDatabase().deleteAllRouters();
    }

    void Router::setConfigValue(const std::string& key, const std::string& value)
    {
        m_impl->ctx.getDatabase().setConfigValue(key, value);
    }

    std::string Router::getConfigValue(const std::string& key)
    {
        return m_impl->ctx.getDatabase().getConfigValue(key);
    }
}

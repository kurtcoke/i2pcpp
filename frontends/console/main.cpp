/**
 * @file main.cpp
 * @brief Contains the starting point, main.
 */
#include "Logger.h"
#include "Server.h"

#include <i2pcpp/Router.h>
#include <i2pcpp/Version.h>
#include <i2pcpp/Database.h>
#include <i2pcpp/Callbacks.h>

#include <i2pcpp/transports/SSU.h>
#include <i2pcpp/datatypes/RouterInfo.h>
#include <i2pcpp/datatypes/Endpoint.h>
#include <i2pcpp/util/make_unique.h>

#include <boost/filesystem.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <botan/botan.h>
#include <botan/elgamal.h>
#include <botan/dsa.h>

#include <botan/elgamal.h>
#include <botan/dsa.h>
#include <botan/auto_rng.h>

#include <signal.h>
#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <condition_variable>

static volatile bool quit = false;
static volatile bool graceful = false;
static std::condition_variable cv;

static void sighandler(int sig)
{
    quit = true;
    cv.notify_all();
}

static void sig_graceful_shutdown(int sig)
{
    graceful = true;
    quit = true;
    cv.notify_all();
}

int main(int argc, char **argv)
{
    using namespace i2pcpp;
    using namespace std;

    try {
        if(signal(SIGINT, &sighandler) == SIG_ERR) {
            cerr << "error setting up signal handler" << endl;

            return EXIT_FAILURE;
        } else if(signal(SIGUSR1, &sig_graceful_shutdown) == SIG_ERR) {
            cerr << "error setting up signal handler" << endl;
        }

        string dbFile;

        namespace po = boost::program_options;

        po::options_description general("General options");
        general.add_options()
            ("help,h", "Print help")
            ("version,v", "Print application version")
            ("log,l", po::value<string>()->implicit_value("i2p.log"), "Log to file instead of clog")
					  ("debug,d", "allow debug output");
				
        po::options_description dbDesc("Database manipulation");
        dbDesc.add_options()
            ("db", po::value<string>(&dbFile)->default_value("i2p.db"), "Database file to operate on")
            ("init", "Initialize a fresh copy of the database")
            ("import", po::value<string>(), "Import a single routerInfo file")
            ("export", po::value<string>(), "Export your routerInfo file")
            ("importdir", po::value<string>(), "Import all files in the given directory recursively")
            ("wipe", "Delete all stored routers and profiles");

        po::options_description config("Configuration manipulation");
        config.add_options()
            ("get", po::value<string>(), "Retrieve a configuration setting")
            ("set", po::value<vector<string>>()->multitoken(), "Set a configuration setting (key value)");

        po::options_description all_opts;
        all_opts.add(general).add(dbDesc).add(config);

        po::variables_map vm;

        po::store(po::command_line_parser(argc, argv).options(all_opts).run(), vm);
        po::notify(vm);

        if(vm.count("help")) {
            cout << general << endl;
            cout << dbDesc << endl;
            cout << config << endl;

            return EXIT_SUCCESS;
        }

        if(vm.count("version")) {
            cout << CLIENT_NAME + ' ' + CLIENT_BUILD << ' ' << CLIENT_DATE << endl;

            return EXIT_SUCCESS;
        }

        i2pcpp::severity_level log_level = i2pcpp::info;
        if (vm.count("debug")) {
            log_level = i2pcpp::debug;
        }
        Logger l;
        i2p_logger_mt lg(boost::log::keywords::channel = "M");

        if(vm.count("init")) {
            Database::createDb(dbFile);

            I2P_LOG(lg, info) << "database created successfully";

            return EXIT_SUCCESS;
        }

        Router::initialize();
        auto db = std::make_shared<Database>(dbFile);
        Callbacks cb;
        Router r(db, cb);

        if(vm.count("log")) {
            // TODO Log rotation, etc
   					Logger::logToFile(vm["log"].as<string>(), log_level);
        } else {
            Logger::logToConsole(log_level);
        }

        if(vm.count("export")) {
            string file = vm["export"].as<string>();
            ofstream f(file, ios::binary);

            if(f.is_open()) {
                Botan::AutoSeeded_RNG rng;
                
                std::string encryptingKeyPEM = db->getConfigValue("private_encryption_key");
                Botan::DataSource_Memory dsm((unsigned char *)encryptingKeyPEM.data(), encryptingKeyPEM.size());
                std::shared_ptr<Botan::ElGamal_PrivateKey> ekey(dynamic_cast<Botan::ElGamal_PrivateKey *>(Botan::PKCS8::load_key(dsm, rng, (std::string)"")));
                
                std::string signingKeyPEM = db->getConfigValue("private_signing_key");
                Botan::DataSource_Memory dsm2((unsigned char *)signingKeyPEM.data(), signingKeyPEM.size());
                std::shared_ptr< Botan::DSA_PrivateKey> skey(dynamic_cast<Botan::DSA_PrivateKey *>(Botan::PKCS8::load_key(dsm2, rng, (std::string)"")));

                Botan::BigInt encryptionKeyPublic, signingKeyPublic;
                encryptionKeyPublic = ekey->get_y();
                signingKeyPublic = skey->get_y();
                
                ByteArray encryptionKeyBytes = Botan::BigInt::encode(encryptionKeyPublic), signingKeyBytes = Botan::BigInt::encode(signingKeyPublic);
                RouterIdentity ident(encryptionKeyBytes, signingKeyBytes, Certificate());
                RouterHash rh = ident.getHash();
                RouterInfo ri = db->getRouterInfo(rh);
                ByteArray info = ri.serialize();
                f.write((char *)info.data(), info.size());
                f.close();

                return EXIT_SUCCESS;
            } else {
                I2P_LOG(lg, fatal) << "error: could not open " << file << " for writing";

                return EXIT_FAILURE;
            }
        }

        if(vm.count("import")) {
            string file = vm["import"].as<string>();
            ifstream f(file, ios::binary);

            if(f.is_open()) {
                ByteArray ribytes = ByteArray((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
                f.close();
                auto begin = ribytes.cbegin();
                RouterInfo ri = RouterInfo(begin, ribytes.cend());
                db->setRouterInfo(ri);
                I2P_LOG(lg, info) << "successfully imported RouterInfo file with hash " << ri.getIdentity().getHash();

                return EXIT_SUCCESS;
            } else {
                I2P_LOG(lg, fatal) << "error: could not open " << file << " for writing";

                return EXIT_FAILURE;
            }
        }

        if(vm.count("importdir")) {
            namespace fs = boost::filesystem;

            string dir = vm["importdir"].as<string>();
            if(!fs::exists(dir)) {
                I2P_LOG(lg, fatal) << "error: directory " << dir << " does not exist";

                return EXIT_FAILURE;
            }

            vector<RouterInfo> routers;
            fs::recursive_directory_iterator itr(dir), end;
            while(itr != end) {
                if(is_regular_file(*itr)) {
                    ifstream f(itr->path().string(), ios::binary);
                    if(!f.is_open()) continue;

                    ByteArray info = ByteArray((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
                    f.close();
                    I2P_LOG(lg, debug) << "importing " << itr->path().string();
                    auto begin = info.cbegin();
                    auto ri = RouterInfo(begin, info.cend());

                    if(ri.verifySignature())
                        routers.push_back(ri);
                    else
                        I2P_LOG(lg, error) << "failed to import " << itr->path().string() << ": bad signature";
                }

                if(fs::is_symlink(*itr)) itr.no_push();

                try {
                    ++itr;
                } catch(exception &e) {
                    itr.no_push();
                    continue;
                }
            }

            db->setRouterInfo(routers);
            I2P_LOG(lg, info) << "successfully imported " << routers.size() << " routers";

            return EXIT_SUCCESS;
        }

        if(vm.count("wipe")) {
            db->deleteAllRouters();

            return EXIT_SUCCESS;
        }

        if(vm.count("get")) {
            cout << db->getConfigValue(vm["get"].as<string>()) << endl;

            return EXIT_SUCCESS;
        }

        if(!vm["set"].empty()) {
            vector<string> tokens;
            if((tokens = vm["set"].as<vector<string>>()).size() != 2) {
                I2P_LOG(lg, fatal) << "error: exactly two arguments must be provided to --set: '--set name value'";

                return EXIT_FAILURE;
            }

            db->setConfigValue(tokens[0], tokens[1]);

            return EXIT_SUCCESS;
        }

        std::unique_ptr<Server> s;
        if(db->getConfigValue("control_server") == "1") {
            Endpoint ep(db->getConfigValue("control_server_ip"), stoi(db->getConfigValue("control_server_port")));
            s = std::make_unique<Server>(ep);

            I2P_LOG(lg, info) << "starting control server";
            s->run();
            I2P_LOG(lg, info) << "started control server";
        }

        Botan::AutoSeeded_RNG rng;

        Botan::DataSource_Stream ekds("encryption.key");
        auto elgPrivKey = std::shared_ptr<Botan::ElGamal_PrivateKey>(dynamic_cast<Botan::ElGamal_PrivateKey *>(Botan::PKCS8::load_key(ekds, rng, (std::string)"")));

        Botan::DataSource_Stream skds("signing.key");
        auto dsaPrivKey = std::shared_ptr<Botan::DSA_PrivateKey>(dynamic_cast<Botan::DSA_PrivateKey *>(Botan::PKCS8::load_key(skds, rng, (std::string)"")));

        Botan::BigInt elgPubKey, dsaPubKey;
        elgPubKey = elgPrivKey->get_y();
        dsaPubKey = dsaPrivKey->get_y();

        ByteArray elgPubKeyBytes = Botan::BigInt::encode(elgPubKey);
        ByteArray dsaPubKeyBytes = Botan::BigInt::encode(dsaPubKey);
        RouterIdentity ri(elgPubKeyBytes, dsaPubKeyBytes, Certificate());

        I2P_LOG(lg, info) << "loading transport";

        auto t = std::make_shared<SSU::SSU>(dsaPrivKey, ri);
        r.addTransport(t);

        I2P_LOG(lg, info) << "starting router";
        r.start();
        t->start(Endpoint(db->getConfigValue("ssu_bind_ip"), std::stoi(db->getConfigValue("ssu_bind_port"))));

        std::mutex mtx;
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return quit; });
        if ( graceful )  {
            I2P_LOG(lg, info) << "doing graceful shutdown";
            r.gracefulShutdown();
            std::chrono::seconds sleep_duration(10);
            while ( r.isActive() ) { 
                I2P_LOG(lg, debug) << "waiting for tunnels to expire";
                std::this_thread::sleep_for(sleep_duration); 
            }
        }

        I2P_LOG(lg, debug) << "shutting down";

        r.stop();

        if(s)
            s->stop();

        return EXIT_SUCCESS;
    } catch(boost::program_options::error &e) {
        cerr << "error parsing command line arguments: " << e.what() << endl;

        return EXIT_FAILURE;
    } catch(std::exception &e) {
        cerr << "error: " << e.what() << endl;
        
        return EXIT_FAILURE;
    }
}

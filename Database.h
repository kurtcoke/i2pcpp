#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <forward_list>

#include <sqlite3.h>

#include "datatypes/ByteArray.h"
#include "datatypes/RouterInfo.h"

namespace i2pcpp {
	class Database {
		public:
			Database(std::string const &file);
			Database(const Database &) = delete;
			Database& operator=(Database &) = delete;
			~Database();

			static void createDb(std::string const &file);
			std::string getConfigValue(std::string const &name);
			void setConfigValue(std::string const &name, std::string const &value);
			ByteArray getConfigBlob(std::string const &name);
			RouterHash getRandomRouter();
			bool routerExists(RouterHash const &routerHash);
			RouterInfo getRouterInfo(std::string const &routerHash);
			RouterInfo getRouterInfo(RouterHash const &routerHash);
			void deleteRouter(RouterHash const &hash);
			void deleteAllRouters();
			void setRouterInfo(std::vector<RouterInfo> const &routers);
			void setRouterInfo(RouterInfo const &info, bool transaction = true);
			std::forward_list<RouterHash> getAllHashes();

		private:
			sqlite3 *m_db;

			mutable std::mutex m_mutex;
	};
}

#endif

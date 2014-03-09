/**
 * @file ProfileManager.cpp
 * @brief Implements ProfileManager.h
 */
#include "ProfileManager.h"

#include "RouterContext.h"

namespace i2pcpp {
    ProfileManager::ProfileManager(RouterContext &ctx) :
        m_ctx(ctx) {}

    const RouterInfo ProfileManager::getPeer()
    {
        return m_ctx.getDatabase().getRouterInfo(m_ctx.getDatabase().getRandomRouter());
    }
}

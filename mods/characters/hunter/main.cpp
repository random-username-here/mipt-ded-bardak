#include "AssetManager.hpp"
#include "BmServerModule.hpp"
#include "Map.hpp"
#include "RoleMgr.hpp"
#include "Timer.hpp"
#include "hunter_manager.hpp"
#include "binmsg.hpp"
#include "modlib_manager.hpp"
#include "modlib_mod.hpp"

#include <string_view>

using namespace modlib;

class HunterModule final : public BmServerModule {
    Timer   *tm    = nullptr;
    Level   *map   = nullptr;
    RoleMgr *roles = nullptr;
    anim::AnimationManager *animator = nullptr;
    AssetManager           *assets   = nullptr;
    HunterManager           manager;

public:
    std::string_view id()    const override { return "ashww.bardak.uctl.hunter"; }
    std::string_view brief() const override { return "Unit controller for hunter"; }
    ModVersion version()     const override { return ModVersion(0, 0, 1); }

    void onResolveDeps(ModManager *mm) override
    {
        tm       = mm->anyOfType<Timer>();
        map      = mm->anyOfType<Level>();
        roles    = mm->anyOfType<RoleMgr>();
        animator = mm->anyOfType<anim::AnimationManager>();
        assets   = mm->anyOfType<AssetManager>();

        if (!tm) {
            throw ModManager::Error("Timer module not found");
        }
        if (!map) {
            throw ModManager::Error("Map module not found");
        }
        if (!roles) {
            throw ModManager::Error("RoleMgr module not found");
        }
        if (!animator) {
            throw ModManager::Error("Animator module not found");
        }
        if (!assets) {
            throw ModManager::Error("AssetManager module not found");
        }

        manager.setModules(tm, map, animator, assets);
    }

    void onDepsResolved(ModManager *) override
    {
        manager.resolve();

        if (!roles->registerRole("hunter", "hunter", "hunter", [this](BmClient *client) {
                select(client);
            })) {
            throw ModManager::Error("failed to register hunter role");
        }
    }

    void onSetup(BmServer *server) override
    {
        server->registerPrefix("hunter", this);
    }

    void onConnect(BmClient *) override {}

    void onDisconnect(BmClient *client) override
    {
        manager.destroy(client);
    }

    void onMessage(BmClient *client, bmsg::RawMessage message) override
    {
        if (!message.isCorrect() || !roles->clientHasRole(client, "hunter")) {
            return;
        }

        if (message.header()->type == "move") {
            const auto move = bmsg::CL_hunter_move::decode(message);
            if (move) {
                manager.receiveMoveCommand(client, *move);
            }
            return;
        }

        if (message.header()->type == "use") {
            const auto use = bmsg::CL_hunter_use::decode(message);
            if (use) {
                manager.receiveUseCommand(client, *use);
            }
            return;
        }
    }

private:
    void select(BmClient *client)
    {
        if (manager.count(client) != 0) {
            return;
        }

        manager.spawnHunter(client);
    }
};

extern "C" Mod *modlib_create(ModManager *)
{
    return new HunterModule();
}

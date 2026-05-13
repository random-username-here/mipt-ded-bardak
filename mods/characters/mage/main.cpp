#include "AssetManager.hpp"
#include "BmServerModule.hpp"
#include "Map.hpp"
#include "RoleMgr.hpp"
#include "Roots.hpp"
#include "Timer.hpp"
#include "binmsg.hpp"
#include "mage_manager.hpp"
#include "modlib_manager.hpp"
#include "modlib_mod.hpp"

#include <string_view>

using namespace modlib;

class MageModule final : public BmServerModule {
    Timer      *tm    = nullptr;
    Level      *map   = nullptr;
    RootSystem *roots = nullptr;
    RoleMgr    *roles = nullptr;
    anim::AnimationManager *animator = nullptr;
    AssetManager           *assets   = nullptr;
    MageManager             manager;

public:
    std::string_view id()    const override { return "ashww.bardak.uctl.mage"; }
    std::string_view brief() const override { return "Unit controller for mage"; }
    ModVersion version()     const override { return ModVersion(0, 0, 1); }

    void onResolveDeps(ModManager *mm) override
    {
        tm       = mm->requireAnyOfType<Timer>("Mage needs Timer");
        map      = mm->requireAnyOfType<Level>("Mage needs Map");
        roots    = mm->requireAnyOfType<RootSystem>("Mage needs Roots");
        roles    = mm->requireAnyOfType<RoleMgr>("Mage needs RoleMgr");
        animator = mm->requireAnyOfType<anim::AnimationManager>("Mage needs Animator");
        assets   = mm->requireAnyOfType<AssetManager>("Mage needs AssetManager");

        manager.setModules(tm, map, roots, animator, assets);
    }

    void onDepsResolved(ModManager *) override
    {
        manager.resolve();

        if (!roles->registerRole("mage", "mage", "mage", [this](BmClient *client) {
                select(client);
            })) {
            throw ModManager::Error("failed to register mage role");
        }
    }

    void onSetup(BmServer *server) override
    {
        server->registerPrefix("mage", this);
    }

    void onConnect(BmClient *) override {}

    void onDisconnect(BmClient *client) override
    {
        manager.destroy(client);
    }

    void onMessage(BmClient *client, bmsg::RawMessage message) override
    {
        if (!message.isCorrect() || !roles->clientHasRole(client, "mage")) {
            return;
        }

        if (message.header()->type == "move") {
            const auto move = bmsg::CL_mage_move::decode(message);
            if (move) {
                manager.receiveMoveCommand(client, *move);
            }
            return;
        }

        if (message.header()->type == "use") {
            const auto use = bmsg::CL_mage_use::decode(message);
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

        manager.spawnMage(client);
    }
};

extern "C" Mod *modlib_create(ModManager *)
{
    return new MageModule();
}

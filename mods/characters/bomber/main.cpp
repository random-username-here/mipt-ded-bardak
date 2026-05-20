#include "AssetManager.hpp"
#include "BmServerModule.hpp"
#include "Map.hpp"
#include "RoleMgr.hpp"
#include "Timer.hpp"
#include "bomber_manager.hpp"
#include "binmsg.hpp"
#include "modlib_manager.hpp"
#include "modlib_mod.hpp"

#include "bomb/bomb.hpp"

#include <string_view>

using namespace modlib;

class BomberModule final : public BmServerModule {
    Timer   *tm    = nullptr;
    Level   *map   = nullptr;
    RoleMgr *roles = nullptr;
    anim::AnimationManager *animator = nullptr;
    AssetManager           *assets   = nullptr;
    bombs::BombsModule     *bombs_module = nullptr;
    BomberManager           manager;

public:
    std::string_view id()    const override { return "dinichthys.bardak.uctl.bomber"; }
    std::string_view brief() const override { return "Unit controller for bomber"; }
    ModVersion version()     const override { return ModVersion(0, 0, 1); }

    void onResolveDeps(ModManager *mm) override
    {
        tm       = mm->anyOfType<Timer>();
        map      = mm->anyOfType<Level>();
        roles    = mm->anyOfType<RoleMgr>();
        animator = mm->anyOfType<anim::AnimationManager>();
        assets   = mm->anyOfType<AssetManager>();
        bombs_module = mm->anyOfType<bombs::BombsModule>();

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
        if (!bombs_module) {
            throw ModManager::Error("BombsModule module not found");
        }

        manager.setModules(tm, map, animator, assets);
    }

    void onDepsResolved(ModManager *mm) override
    {
        if (!bombs_module) {
            bombs_module = mm->anyOfType<bombs::BombsModule>();
            if (!bombs_module) {
                throw ModManager::Error("BombsModule module not found");
            }
        }

        manager.resolve();

        if (!roles->registerRole("bomber", "bomber", "bomber", [this](BmClient *client) {
                select(client);
            })) {
            throw ModManager::Error("failed to register bomber role");
        }
    }

    void onSetup(BmServer *server) override
    {
        server->registerPrefix("bomber", this);
    }

    void onConnect(BmClient *) override {}

    void onDisconnect(BmClient *client) override
    {
        manager.destroy(client);
    }

    void onMessage(BmClient *client, bmsg::RawMessage message) override
    {
        if (!message.isCorrect() || !roles->clientHasRole(client, "bomber")) {
            return;
        }

        if (message.header()->type == "move") {
            const auto move = bmsg::CL_bomber_move::decode(message);
            if (move) {
                manager.receiveMoveCommand(client, *move);
            }
            return;
        }

        if (message.header()->type == "bomb") {
            const auto bomb = bmsg::CL_bomber_bomb::decode(message);
            if (bomb) {
                manager.receiveBombCommand(client, *bomb, bombs_module);
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

        manager.spawnBomber(client);
    }
};

extern "C" Mod *modlib_create(ModManager *)
{
    return new BomberModule();
}

#include "AssetManager.hpp"
#include "BmServerModule.hpp"
#include "Map.hpp"
#include "RoleMgr.hpp"
#include "Timer.hpp"
#include "binmsg.hpp"
#include "tank_manager.hpp"
#include "modlib_manager.hpp"
#include "modlib_mod.hpp"

#include <optional>
#include <string_view>

using namespace modlib;

class TankModule final : public BmServerModule {
    Timer   *tm    = nullptr;
    Level   *map   = nullptr;
    RoleMgr *roles = nullptr;
    anim::AnimationManager *animator = nullptr;
    AssetManager           *assets   = nullptr;
    TankManager manager;

public:
    std::string_view id()    const override { return "ida.bardak.uctl.tank"; }
    std::string_view brief() const override { return "Unit controller for tank"; }
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
        if (!roles->registerRole("tank", "tank", "tank", [this](BmClient *client) {
                select(client);
            })) {
            throw ModManager::Error("failed to register tank role");
        }
    }

    void onSetup(BmServer *server) override
    {
        server->registerPrefix("tank", this);
    }

    void onConnect(BmClient *) override {}

    void onDisconnect(BmClient *client) override
    {
        manager.destroy(client);
    }

    void onMessage(BmClient *client, bmsg::RawMessage message) override
    {
        if (!message.isCorrect() || !roles->clientHasRole(client, "tank")) {
            return;
        }

        if (message.header()->type == "move") {
            const auto move = bmsg::CL_tank_move::decode(message);
            if (move) manager.receiveMoveCommand(client, *move);
            return;
        }

        if (message.header()->type == "rotate") {
            const auto rotate = bmsg::CL_tank_rotate::decode(message);
            if (rotate) manager.receiveRotateCommand(client, *rotate);
            return;
        }

        if (message.header()->type == "shoot") {
            const auto shoot = bmsg::CL_tank_shoot::decode(message);
            if (shoot) manager.receiveShootCommand(client, *shoot);
            return;
        }
    }

private:
    void select(BmClient *client)
    {
        if (manager.count(client) != 0) {
            return;
        }
        manager.spawnTank(client);
    }
};

extern "C" Mod *modlib_create(ModManager *)
{
    return new TankModule();
}

#include "Map.hpp"
#include <cstdlib>
#include <optional>

#include "BmServerModule.hpp"
#include "Map.hpp"
#include "Timer.hpp"
#include "binmsg.hpp"
#include "modlib_mod.hpp"
#include "modlib_manager.hpp"
#include "AssetManager.hpp"
#include "AssetConfig.hpp"
#include <cstdlib>
#include <iostream>
#include <optional>

#include "person_manager.hpp"

using namespace modlib;

class PersonModule : public BmServerModule {   
    Timer        *tm=nullptr;
    Map          *map=nullptr;
    AssetManager *assetManager=nullptr;

    PersonManager manager;

public:
    std::string_view id() const override { return "isd.bardak.uctl.person"; };
    std::string_view brief() const override { return "Unit controller for a simple man walking around"; };
    ModVersion version() const override { return ModVersion(0, 0, 1); };

    void onResolveDeps(ModManager *mm) override {
        tm = mm->anyOfType<Timer>();
        map = mm->anyOfType<Map>();
        assetManager = mm->anyOfType<AssetManager>();
        if (!tm) throw ModManager::Error("Timer module not found");
        if (!map) throw ModManager::Error("Map module not found");
        if (!assetManager) throw ModManager::Error("AssetManager module not found");

        manager.setModules(tm, map, assetManager);
    }   

    void onDepsResolved(ModManager *mm) override {
        manager.resolve();
    }

    void onSetup(BmServer *server) override {
        server->registerPrefix("person", this);
    };

    void onConnect(BmClient *client) override {
        manager.spawnPerson(client);
    }

    void onMessage(BmClient *cl, bmsg::RawMessage m) override {
        assert(m.isCorrect());
        if (m.header()->type == "move") {
            auto moveCmd = bmsg::CL_person_move::decode(m);
            if (moveCmd == std::nullopt) return;
            manager.receiveMoveCommand(cl, moveCmd.value());          
        } else if (m.header()->type == "attack") {
            auto atkCmd = bmsg::CL_person_attack::decode(m);
            if (!atkCmd) return;
            manager.receiveAttackCommand(cl, atkCmd.value());
        }
        manager.receiveAcionDone(cl);
    }

    void onDisconnect(BmClient *client) override {
        manager.destroy(client);
    }
};

extern "C" Mod *modlib_create(ModManager *mm) {
    return new PersonModule();
}

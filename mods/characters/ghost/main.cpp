#include <cassert>
#include <iostream>

#include "BmServerModule.hpp"
#include "Animator.hpp"
#include "AssetManager.hpp"
#include "RoleMgr.hpp"
#include "Map.hpp"
#include "Timer.hpp"
#include "binmsg.hpp"
#include "modlib_mod.hpp"
#include "modlib_manager.hpp"

#include "ghost_manager.hpp"

using namespace modlib;

class GhostModule : public BmServerModule
{
	Timer *tm = nullptr;
	Level *map = nullptr;
	RoleMgr *roles_ = nullptr;
	anim::AnimationManager *animator = nullptr;
	AssetManager *assets = nullptr;
	GhostManager manager;

public:
	std::string_view id() const override
	{
		return "sevsol.bardak.uctl.ghost";
	}
	std::string_view brief() const override
	{
		return "Ghost unit controller";
	}
	ModVersion version() const override
	{
		return ModVersion(0, 1, 0);
	}

	void onResolveDeps(ModManager *mm) override
	{
		tm = mm->anyOfType<Timer>();
		map = mm->anyOfType<Level>();
		roles_ = mm->anyOfType<modlib::RoleMgr>();
		animator = mm->anyOfType<anim::AnimationManager>();
		assets = mm->anyOfType<AssetManager>();

		if (!tm) {
			throw ModManager::Error("Timer module not found");
		}
		if (!map) {
			throw ModManager::Error("Map module not found");
		}
		if (!roles_) {
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

	void select(modlib::BmClient *client)
	{
		if (manager.count(client) != 0) {
			return;
		}
		manager.spawnGhost(client);
	}

	void onDepsResolved(ModManager * /*mm*/) override
	{
		manager.resolve();
		if (!roles_->registerRole("ghost", "Ghost", "ghost",
		                          [this](modlib::BmClient *client) { select(client); })) {
			throw ModManager::Error("failed to register ghost role");
		}
	}

	void onSetup(BmServer *server) override
	{
		if (!server->registerPrefix("ghost", this)) {
			throw ModManager::Error("failed to register ghost prefix");
		}
	}

	void onConnect(BmClient * /*client*/) override {}

	void onMessage(BmClient *cl, bmsg::RawMessage m) override
	{
		assert(m.isCorrect());

		if (!roles_->clientHasRole(cl, "ghost") || !m.isCorrect()) {
			return;
		}

		if (m.header()->type == "move") {
			const auto move_cmd = bmsg::CL_ghost_move::decode(m);
			if (!move_cmd) {
				return;
			}
			manager.receiveMoveCommand(cl, move_cmd.value());
		} else if (m.header()->type == "attack") {
			const auto atk_cmd = bmsg::CL_ghost_attack::decode(m);
			if (!atk_cmd) {
				return;
			}
			manager.receiveAttackCommand(cl, atk_cmd.value());
		}
	}

	void onDisconnect(BmClient *client) override
	{
		manager.destroy(client);
	}
};

extern "C" Mod *modlib_create(ModManager *)
{
	return new GhostModule();
}
